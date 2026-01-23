Snapshot handling
^^^^^^^^^^^^^^^^^


Current architecture: Initialization after successful shutdown
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

.. uml::

  skinparam actorStyle awesome

  actor "User App" as APP
  box "score::mw::per"
      participant "Frontend" as API
      participant "Backend" as BACKEND
  end box

  APP -> API: KvsBuilder(instance_id: InstanceId)
  API -> BACKEND: load_kvs(1)
  API <- BACKEND: KvsMap
  APP <- API: kvs.has_value()
  APP -> API: get_value(key: Key)
  API -> API: Value = KvsMap[Key]
  APP <- API: Value

  note over APP, API: ...

  APP -> API: flush()
  API -> BACKEND: flush(KvsMap)
  BACKEND -> BACKEND: snapshot_rotate()
  BACKEND -> BACKEND: write_kvs(1)
  API <- BACKEND:
  APP <- API:


Current architecture: Initialization after interrupted shutdown
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

.. uml::

  skinparam actorStyle awesome

  actor "User App" as APP
  box "score::mw::per"
    participant "Frontend" as API
    participant "Backend" as BACKEND
  end box

  APP -> API: KvsBuilder(instance_id: InstanceId)
  API -> BACKEND: load_kvs(1)
  API <- BACKEND: Error
  APP <- API: kvs.has_value()
  note over APP: Do we have to return value in error case?\nJust enable call of snapshot_restore() later?
  alt Recoverable error  (eg. ValidationFailed)
    loop until valid snapshot found or no more snapshots
      APP -> API: kvs.snapshot_restore(shapshot_id++)
      API -> BACKEND: load_kvs(shapshot_id)
      API <- BACKEND: KvsMap
    end
  else Non recoverable error (eg. disk error)
    note over APP: Error handling ...
  end


New snapshot handling  proposal
"""""""""""""""""""""""""""""""

- Current solution is too low level and requires to implement error handling and recovery loop in every application.
- All applications will use the same recovery loop, so it makes sense to move it into the mw::per library.
- Majority of applications doesn't need to know whether the last shutdown was successful or not. They just want a valid KVS instance, even if they are getting data from previous successfully accomplished transaction
- If the application need to know; we can introduce new public API to query the last shutdown status.
- Storing several snapshots as complete copy is consuming disk space, without any improvement of robustness.
- Snapshots shall not be used to achieve transactional safe write operations.
- Snapshots shall be used to create point in time frozen view on the values.
- Snapshots shall be created, restored and deleted explicitly by the application.
- Example use cases:

  -  before applying an update, create a snapshot. If the update fails, restore the snapshot.
  -  store optimized, normal and "relaxed" timings for time sensible applications.

.. uml::

  skinparam actorStyle awesome

  actor "User App" as APP
  box "score::mw::per"
    participant "Frontend" as API
    participant "Backend" as BACKEND
  end box

  APP -> API: KvsBuilder(instance_id: InstanceId, SnapshotID)
    alt Write transaction completed
      API -> BACKEND: load_last_kvs()
    else Write transaction interrupted
      API -> BACKEND: load_previous_kvs()
    end
    API <- BACKEND: KvsMap
  APP <- API: kvs.has_value()

  APP -> API: get_value(key: Key)
  API -> API: Value = KvsMap[Key]
  APP <- API: Value
  group SW Update
    APP -> API: create_snapshot("Before update")
    API -> BACKEND: create_snapshot(KvsMap, "Before update")
    API <- BACKEND:
    APP <- API:

    APP -> API: set_value(key: Key, value: NewValue)
    API -> API: KvsMap[Key] = NewValue
    APP <- API:
    note over APP, API: ...
  end

  APP -> API: get_value(key: Key)
  API -> API: NewValue = KvsMap[Key]
  APP <- API: NewValue

  group SW Update Rollback
    APP -> API: restore_snapshot("Before update")
    API -> BACKEND: restore_snapshot("Before update")
    API <- BACKEND: KvsMap
    APP <- API:
    APP -> API: delete_snapshot("Before update")
      API -> BACKEND: delete_snapshot("Before update")
      API <- BACKEND:
    APP <- API:
  end

  APP -> API: get_value(key: Key)
  API -> API: Value = KvsMap[Key]
  APP <- API: Value

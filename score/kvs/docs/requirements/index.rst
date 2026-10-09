..
   # *******************************************************************************
   # Copyright (c) 2025 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************

Requirements
############

.. document:: KVS Requirements
   :id: doc__kvs_requirements
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp[version==1]

Component Requirements
----------------------

.. comp_req:: Language Bindings
   :id: comp_req__kvs__language_bindings
   :reqtype: Non-Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__persistency__cpp_rust[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide API bindings for both the C++ and Rust programming languages.

.. comp_req:: Operating System Abstraction
   :id: comp_req__kvs__os_abstraction
   :reqtype: Non-Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__persistency__os_agnostic[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall access operating system services exclusively through an abstraction layer and shall not depend on any specific operating system.

.. comp_req:: No Dynamic Memory Allocation at Runtime
   :id: comp_req__kvs__no_dynamic_memory
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__dynamic_memory_alloc[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall not allocate dynamic memory during runtime operations. All memory required for runtime operation shall be allocated during initialization of a KVS instance.

.. comp_req:: Multi-Instance
   :id: comp_req__kvs__multi_instance
   :reqtype: Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__persistency__multiple_kvs[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall manage all runtime variables within an instance to
   enable creation and use of multiple KVS instances within a
   single software architecture element.

.. comp_req:: Single Process Access
   :id: comp_req__kvs__single_process_access
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__multiple_app[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall prevent a single KVS instance from being opened concurrently by more than one OS process.

.. comp_req:: Permission Control
   :id: comp_req__kvs__permission_control
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__access_control[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall rely on the underlying filesystem for access and
   permission management and shall not implement its own access or permission
   controls.

.. comp_req:: Permission Error Handling
   :id: comp_req__kvs__permission_err_hndl
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__access_control[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall report any access or permission errors encountered at
   the filesystem level to the application.

.. comp_req:: Storage File Permissions
   :id: comp_req__kvs__storage_file_permissions
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__access_control[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall create storage files with file permissions ``640`` (owner read and write, group read, others none).

.. comp_req:: Default Storage File Permissions
   :id: comp_req__kvs__default_storage_permissions
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__access_control[version==1],feat_req__persistency__tooling[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall generate the default-values storage file, via the provided kvs tool, with read-only file permissions ``440`` (owner and group read, others none), so that the default storage deployed with the application cannot be modified at runtime.

.. comp_req:: Constraint Configuration
   :id: comp_req__kvs__constraints
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__cfg[version==2]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall allow configuration of KVS constraints at compile-time
   using source code constants or at runtime using a configuration file.

.. comp_req:: Constraint Configuration priority
   :id: comp_req__kvs__constraints_priority
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__cfg[version==2],feat_req__persistency__cfg_priority[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall give configurations provided by file higher priority than compile-time configurations.

.. comp_req:: Maximum Number of Snapshots
   :id: comp_req__kvs__snapshot_max_num_cfg
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__cfg[version==2]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall maintain a configurable maximum number of snapshots.
   The maximum number shall be in the range ``<0..3>``.
   A value of zero shall disable snapshot operations.
   A non-zero value shall specify the maximum number of snapshots.
   Default value shall be: ``3``.

.. comp_req:: Key Naming
   :id: comp_req__kvs__key_naming
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_keys[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall accept keys that consist solely of alphanumeric characters, underscores, or dashes.

.. comp_req:: Key Encoding
   :id: comp_req__kvs__key_encoding
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_keys[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall encode each key as valid UTF-8.

.. comp_req:: Key Uniqueness
   :id: comp_req__kvs__key_uniqueness
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_keys[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall guarantee that each key is unique.

.. comp_req:: Key Value Overwrite
   :id: comp_req__kvs__key_value_overwrite
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_keys[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall overwrite the existing value with the new value when a value is set for a key that already exists.

.. comp_req:: Key Length
   :id: comp_req__kvs__key_length
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_keys[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall limit the maximum length of a key to 32 bytes.

.. comp_req:: Key Existence Check
   :id: comp_req__kvs__key_exists_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_keys[version==1],feat_req__persistency__cached_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]

   The component shall provide a capability to report whether a key has been explicitly written, returning ``false`` for a key that was never written even if a default value is available for it.

.. comp_req:: Key Listing
   :id: comp_req__kvs__key_list_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_keys[version==1],feat_req__persistency__cached_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]

   The component shall provide a capability to retrieve the set of keys that have been explicitly written, excluding keys that only have a default value.

.. comp_req:: Key Removal
   :id: comp_req__kvs__key_remove_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_keys[version==1],feat_req__persistency__cached_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]

   The component shall provide a capability to delete a single explicitly-written key-value pair from the in-memory cache.

.. comp_req:: Value Data Types
   :id: comp_req__kvs__value_data_types
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_value[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall accept only values of the following data types: Number,
   String, Boolean, Null, Array[Value], or Dictionary{Key:Value}.

.. comp_req:: Value Retrieval
   :id: comp_req__kvs__value_get_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_value[version==1],feat_req__persistency__cached_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]

   The component shall provide a capability to retrieve the in-memory value associated with a key, returning the default value (see :need:`comp_req__kvs__value_default`) when the key has not been explicitly written.

.. comp_req:: Value Store
   :id: comp_req__kvs__value_set_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_value[version==1],feat_req__persistency__cached_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]

   The component shall provide a capability to store a value for a given key in the in-memory cache, overwriting any previously written value for that key (see :need:`comp_req__kvs__key_value_overwrite`).

.. comp_req:: Value Serialization
   :id: comp_req__kvs__value_serialize
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_value[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall serialize and deserialize all values to and from JSON.

.. comp_req:: Value Default
   :id: comp_req__kvs__value_default
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_value[version==1],feat_req__persistency__default_values[version==1],feat_req__persistency__default_value_get[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide an API ``get_default_value`` to retrieve the default value of a key when no value has been set by the user.

.. comp_req:: Value Reset
   :id: comp_req__kvs__value_reset
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__support_datatype_value[version==1],feat_req__persistency__default_values[version==1],feat_req__persistency__reset_to_default[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide an API to reset a value to its default when a default value is defined.

   .. note::
      The default value can be retrieved via the ``get_default_value`` API.

.. comp_req:: Default Value Datatypes
   :id: comp_req__kvs__default_value_types
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__default_values[version==1],feat_req__persistency__support_datatype_value[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall accept default values of only the value data types specified in :need:`comp_req__kvs__value_data_types`.

.. comp_req:: Default Value Query
   :id: comp_req__kvs__default_value_query
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__default_values[version==1],feat_req__persistency__default_value_get[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide an API to retrieve default values even if value is set by the user.

.. comp_req:: Default Value Status Query
   :id: comp_req__kvs__default_value_is_default_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__default_values[version==1],feat_req__persistency__default_value_get[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]

   The component shall provide a capability to report whether a key has not been explicitly written and a default value is available for it.

.. comp_req:: Default Value Config
   :id: comp_req__kvs__default_value_cfg
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__default_values[version==1],feat_req__persistency__default_value_file[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall allow configuration of default values in a
   separate storage file generated by provided kvs tool.

.. comp_req:: Default Value Checksum
   :id: comp_req__kvs__default_val_chksum
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__default_value_file[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall secure the configuration file for default values with an
   associated checksum file when default values are stored in a file.

.. comp_req:: Reset Resistant Write
   :id: comp_req__kvs__reset_resistant_write
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__reset_resistant[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall perform flush operations such that the previously persisted consistent data remains intact until a new flush operation has completed successfully.

.. comp_req:: Corruption Detection on Open
   :id: comp_req__kvs__corruption_detect_open
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__recovery_from_reset[version==1],feat_req__persistency__integrity_check[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall detect an interrupted or corrupted storage file, via its integrity check, when a KVS instance is opened, and shall not automatically load the affected storage.

.. comp_req:: Open Behavior on Missing or Corrupted Storage
   :id: comp_req__kvs__open_need_kvs_behavior
   :reqtype: Functional
   :security: YES
   :safety: ASIL_B
   :derived_from: feat_req__persistency__recovery_from_reset[version==1],feat_req__persistency__cfg[version==2]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall behave according to the ``Need Kvs`` configuration parameter (represented by the ``OpenNeedKvs`` flag) when opening a KVS instance whose storage is missing or fails the integrity check:

   - ``Required``: the component shall report an error;
   - ``Optional``: the component shall report an error when the storage file exists but fails the integrity check, and shall open the KVS instance with an empty storage when no storage file exists;
   - ``Ignore``: the component shall open the KVS instance with an empty storage.

.. comp_req:: Recovery via Snapshot Restore
   :id: comp_req__kvs__recovery_snapshot_restore
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__recovery_from_reset[version==1],feat_req__persistency__snapshot_restore[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall allow the user to explicitly restore a snapshot of their choice in order to recover the data after an interrupted or corrupted storage has been detected.

.. comp_req:: Atomic Store
   :id: comp_req__kvs__atomic_store
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__atomic_store[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall persist all key-value pairs of a flush operation atomically, such that either all changes are written or none are.

.. comp_req:: Explicit Flush
   :id: comp_req__kvs__explicit_flush
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__store_data[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall persist in-memory key-value changes to storage only when the user explicitly calls the flush operation, and shall not flush automatically.

.. comp_req:: Write Amplification Minimization
   :id: comp_req__kvs__write_amplification
   :reqtype: Non-Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__write_amplification[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall write key-value data to the storage backend only when the in-memory data has changed since the last successful flush operation.

.. comp_req:: Cached Access
   :id: comp_req__kvs__cached_access
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__cached_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall hold key-value pairs in an in-memory cache and shall serve read and write access to key-value pairs from that cache.

.. comp_req:: Store Reset
   :id: comp_req__kvs__store_reset_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__cached_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]

   The component shall provide a capability to remove all explicitly-written key-value pairs from the in-memory cache, restoring the KVS instance to its initial, empty state.

.. comp_req:: Discard Pending Changes
   :id: comp_req__kvs__discard_pending_changes_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__load_data[version==1],feat_req__persistency__cached_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]

   The component shall provide a capability to reload key-value pairs from persistent storage, discarding every in-memory change made since the last successful flush operation (or since open, if flush was never called), without affecting default values.

.. comp_req:: Persistent Data Storage Checksum Write
   :id: comp_req__kvs__pers_data_csum
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__integrity_check[version==1],feat_req__persistency__store_data[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall generate a checksum for each data file and shall store
   it alongside the data.

.. comp_req:: Persistent Data Storage Checksum Verify
   :id: comp_req__kvs__pers_data_csum_vrfy
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__integrity_check[version==1],feat_req__persistency__load_data[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall verify the checksum when loading data, and shall report an error and not load the affected data when the checksum does not match.

.. comp_req:: Persistent Data Storage Backend
   :id: comp_req__kvs__pers_data_store_bnd
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__integrity_check[version==1],feat_req__persistency__store_data[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall use the file API to persist data.

.. comp_req:: Persistent Data Storage Format
   :id: comp_req__kvs__pers_data_store_fmt
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__integrity_check[version==1],feat_req__persistency__store_data[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall use the JSON data format to persist data.

.. comp_req:: Storage Backend Selection
   :id: comp_req__kvs__storage_backend_select
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__storage_backends[version==2]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall allow the storage backend used by a KVS instance to be selected via a compile-time configuration parameter.

.. comp_req:: Async API
   :id: comp_req__kvs__async_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__async_api[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide an asynchronous API for flush function in addition to the standard API.

.. comp_req:: Callback Support
   :id: comp_req__kvs__callback_support
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__async_api[version==1],feat_req__persistency__async_completion[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide an API for registering callbacks that are triggered by data change events.

.. comp_req:: Snapshot Create API
   :id: comp_req__kvs__snapshot_create_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :derived_from: feat_req__persistency__snapshot_create[version==1]
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall provide an API for creating snapshots.
   The API shall accept an argument that selects a snapshot slot by index.
   The snapshot slot shall be in range ``<1..3>``.
   The API shall return an error when provided slot index is out of range or provided slot index is zero.
   See :need:`comp_req__kvs__snapshot_max_num_cfg`.


   .. note::

      A snapshot is a point-in-time, frozen view of all values in a key-value storage.

.. comp_req:: Snapshot Create
   :id: comp_req__kvs__snapshot_create
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :derived_from: feat_req__persistency__snapshot_create[version==1]
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall create a new snapshot in the selected snapshot slot when the slot is empty.

.. comp_req:: Snapshot Overwrite
   :id: comp_req__kvs__snapshot_overwrite
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :derived_from: feat_req__persistency__snapshot_create[version==1]
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall overwrite the selected snapshot slot when the slot is occupied.

.. comp_req:: Explicit Snapshot Operations
   :id: comp_req__kvs__explicit_snapshot_operations
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :derived_from: feat_req__persistency__snapshot_create[version==1], feat_req__persistency__snapshot_restore[version==1], feat_req__persistency__snapshot_remove[version==1]
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall perform snapshot creation, restoration, and deletion only when explicitly triggered by the user through the corresponding APIs.

.. comp_req:: Snapshot Slot Indexing
   :id: comp_req__kvs__snapshot_id_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__snapshot_create[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall identify snapshot slots by a one-based index, where the first slot has index 1, the second slot has index 2, and so on.

.. comp_req:: Snapshot Data Source
   :id: comp_req__kvs__snapshot_source
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :derived_from: feat_req__persistency__snapshot_create[version==1]
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall use the live values set by the user, regardless of whether the values
   have been flushed to disk.

.. comp_req:: Snapshot Slot Free Query API
   :id: comp_req__kvs__snapshot_slot_free_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :derived_from: feat_req__persistency__snapshot_create[version==1], feat_req__persistency__snapshot_remove[version==1]
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall provide an API to check whether a slot identified by a snapshot index is free or occupied.
   The API shall return an error when provided slot index is out of range. See :need:`comp_req__kvs__snapshot_create_api`.

.. comp_req:: Snapshot Restore API
   :id: comp_req__kvs__snapshot_restore_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :derived_from: feat_req__persistency__snapshot_restore[version==1]
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall provide an API for restoring snapshots.
   The API shall accept an argument that selects a snapshot slot by index.
   The function shall return an error when the referenced snapshot slot is free.
   The API shall return an error when provided slot index is out of range. See :need:`comp_req__kvs__snapshot_create_api`.

.. comp_req:: Snapshot Remove API
   :id: comp_req__kvs__snapshot_remove_api
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :derived_from: feat_req__persistency__snapshot_remove[version==1]
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: valid

   The component shall provide an API for removing snapshots.
   The API shall accept an argument that selects a snapshot slot by index.
   The function shall return an error when the referenced slot is free.
   The API shall return an error when provided slot index is out of range. See :need:`comp_req__kvs__snapshot_create_api`.


.. comp_req:: Concurrency
   :id: comp_req__kvs__concurrency
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__concurrency[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall implement thread-safe mechanisms to enable concurrent
   access to data without data races.

.. comp_req:: Persistent Data Versioning
   :id: comp_req__kvs__pers_data_version
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__versioning[version==1],feat_req__persistency__update_mechanism[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall store a schema version identifier in each persisted data
   file.

.. comp_req:: Persistent Data Schema
   :id: comp_req__kvs__pers_data_schema
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__versioning[version==1],feat_req__persistency__update_mechanism[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall use a JSON file storage format in which the schema
   version identifier is a dedicated top-level field, separate from the
   key-value payload.

.. comp_req:: Persistent Data Migration
   :id: comp_req__kvs__pers_data_migration
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__versioning[version==1],feat_req__persistency__update_mechanism[version==1],feat_req__persistency__variant_management[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall migrate persisted data between supported schema versions
   without involvement of the application.

.. comp_req:: Random Access Time Complexity
   :id: comp_req__kvs__random_access_complexity
   :reqtype: Non-Functional
   :security: NO
   :safety: QM
   :derived_from: feat_req__persistency__fast_access[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide random read access to a key-value pair with at most logarithmic time complexity relative to the number of stored key-value pairs.

.. comp_req:: Default Value Config tool
   :id: comp_req__kvs__default_value_cfg_tool
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__tooling[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide a tool to generate the default value storage file and its corresponding checksum file.

.. comp_req:: Storage Tool
   :id: comp_req__kvs__storage_tool
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :derived_from: feat_req__persistency__tooling[version==1]
   :status: valid
   :version: 1
   :satisfied_by: comp__persistency_kvs[version==1]
   :tags: inspected

   The component shall provide a tool to view, modify and validate storage files.


Assumption of Use Requirements
------------------------------

none

Environmental Requirements
--------------------------

none


.. needextend:: c.this_doc() and docname is not None
   :+tags: kvs

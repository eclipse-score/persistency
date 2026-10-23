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


KVS DFA (Dependent Failure Analysis)
====================================

.. document:: KVS DFA
   :id: doc__kvs_dfa
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_component_dfa[version==1]
   :tags: Persistency KVS

Dependent failures at the component interfaces are analysed in :need:`doc__persistency_dfa`. This DFA applies the
dependent failure initiators of :need:`gd_guidl__dfa_failure_initiators` to the mechanisms specified in
:need:`doc__kvs_requirements`. The scope is defined in :need:`doc__persistency_safety_plan` (tailoring).

Dependent Failure Initiators
----------------------------

Shared resources
^^^^^^^^^^^^^^^^

The used libraries (:need:`comp_arc_sta__persistency_kvs__sv`) and the operating system are considered in the
platform DFA.

Communication between the two elements
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Receiving function is affected by information that is false, lost, sent multiple times, or in the wrong order etc. from the sender.

.. list-table:: DFA communication between elements
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause communication between elements
    - Applicability
    - Rationale
  * - CO_01_01
    - Information passed via argument through a function call, or via writing/reading a variable being global to the two software functions (data flow)
    - no
    - Function arguments only; no global data (:need:`comp_req__kvs__multi_instance`).
  * - CO_01_02
    - Data or message corruption / repetition / loss / delay / masquerading or incorrect addressing of information
    - no
    - :need:`comp_saf_fmea__kvs__corrupt_on_open`; other processes: CO_01_04.
  * - CO_01_03
    - Insertion / sequence of information
    - no
    - Data and hash files are written and read as a whole.
  * - CO_01_04
    - Corruption of information, inconsistent data
    - yes
    - :need:`comp_saf_dfa__kvs__file_access`
  * - CO_01_05
    - Asymmetric information sent from a sender to multiple receivers, so that not all defined receivers have the same information
    - no
    - No communication with multiple receivers.
  * - CO_01_06
    - Information from a sender received by only a subset of the receivers
    - no
    - Same as CO_01_05.
  * - CO_01_07
    - Blocking access to a communication channel
    - no
    - No communication channel used.


Shared information inputs
^^^^^^^^^^^^^^^^^^^^^^^^^

Same information input used by multiple functions.

.. list-table:: DFA shared information inputs
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause shared information inputs
    - Applicability
    - Rationale
  * - SI_01_02
    - Configuration data
    - no
    - :need:`comp_saf_fmea__kvs__constraint_cfg_wrong`
  * - SI_01_03
    - Constants, or variables, being global to the two software functions
    - no
    - Data per instance (:need:`comp_req__kvs__multi_instance`); shared instance: :need:`feat_saf_dfa__persistency__shared_instance`.
  * - SI_01_04
    - Basic software passes data (read from hardware register and converted into logical information) to two applications software functions
    - no
    - No hardware access (:need:`comp_req__kvs__os_abstraction`).
  * - SI_01_05
    - Data / function parameter arguments / messages delivered by software function to more than one other function
    - no
    - Values are returned to the calling function only; callbacks: see UI_01_04.


Unintended impact
^^^^^^^^^^^^^^^^^

Unintended impacts to function due to various failures.

.. list-table:: DFA unintended impact
  :header-rows: 1
  :widths: 10,20,10,20

  * - ID
    - Violation cause unintended impact
    - Applicability
    - Rationale
  * - UI_01_01
    - Memory miss-allocation and leaks
    - no
    - Allocation: see UI_01_11. Leaks are addressed by Rust ownership, C++ RAII and component verification.
  * - UI_01_02
    - Read/Write access to memory allocated to another software element
    - no
    - Platform DFA (process isolation).
  * - UI_01_03
    - Stack/Buffer under-/overflow
    - no
    - Rust bounds checking, C++ standard containers, :need:`comp_req__kvs__key_length`.
  * - UI_01_04
    - Deadlocks
    - yes
    - :need:`comp_saf_dfa__kvs__callback_deadlock`
  * - UI_01_05
    - Livelocks
    - no
    - No retry loops; a failed lock is reported (C++ ``MutexLockFailed``).
  * - UI_01_06
    - Blocking of execution
    - no
    - No component specific effect, see :need:`feat_saf_dfa__persistency__execution_blocking`.
  * - UI_01_07
    - Incorrect allocation of execution time
    - no
    - No own execution context except for asynchronous operations; see UI_01_09.
  * - UI_01_08
    - Incorrect execution flow
    - no
    - Execution flow defined by the API; corruption: see UI_01_02.
  * - UI_01_09
    - Incorrect synchronization between software elements
    - yes
    - :need:`comp_saf_dfa__kvs__async_flush`, :need:`comp_saf_dfa__kvs__multi_process`
  * - UI_01_10
    - CPU time depletion
    - no
    - No own execution context except for asynchronous operations, which are started by the application.
  * - UI_01_11
    - Memory depletion
    - yes
    - :need:`comp_saf_dfa__kvs__memory_depletion`
  * - UI_01_12
    - Other HW unavailability
    - no
    - Storage errors are reported (:need:`comp_req__kvs__permission_err_hndl`).


DFA
---

.. comp_saf_dfa:: Storage files modified by other processes
   :violates: comp_arc_sta__persistency_kvs__sv
   :id: comp_saf_dfa__kvs__file_access
   :failure_id: CO_01_04
   :failure_effect: Persisted data, snapshots or default values of a KVS instance are modified by another process.
   :safety_relevant: yes
   :mitigated_by: comp_req__kvs__permission_control, comp_req__kvs__storage_file_permissions, comp_req__kvs__default_storage_permissions, comp_req__kvs__pers_data_csum_vrfy
   :sufficient: yes
   :status: valid
   :version: 1

   Modifications by the kvs tool carry a valid checksum: :need:`comp_saf_fmea__kvs__tool_wrong_output`.

   Implementation: the file permissions are not set.

.. comp_saf_dfa:: Deadlock by callback
   :violates: comp_arc_sta__persistency_kvs__sv
   :id: comp_saf_dfa__kvs__callback_deadlock
   :failure_id: UI_01_04
   :failure_effect: The callback and the KVS instance block each other; the instance is not available.
   :safety_relevant: yes
   :mitigated_by: comp_req__kvs__concurrency, aou_req__persistency__callback
   :sufficient: no
   :status: valid
   :version: 1

   A callback (:need:`comp_req__kvs__callback_support`) executed under the instance lock and calling the same
   instance blocks. Not sufficient: it is not specified that callbacks are executed without holding the lock.

   Implementation: callbacks are missing.

.. comp_saf_dfa:: Concurrent modification during asynchronous flush
   :violates: comp_arc_sta__persistency_kvs__sv
   :id: comp_saf_dfa__kvs__async_flush
   :failure_id: UI_01_09
   :failure_effect: The persisted state is a mixture of the state before and after a concurrent modification.
   :safety_relevant: yes
   :mitigated_by: comp_req__kvs__concurrency, comp_req__kvs__atomic_store, comp_req__kvs__async_api
   :sufficient: yes
   :status: valid
   :version: 1

   The flush persists one consistent state of the cache.

   Implementation: the asynchronous API is missing.

.. comp_saf_dfa:: Access to a KVS instance from multiple processes
   :violates: comp_arc_sta__persistency_kvs__sv
   :id: comp_saf_dfa__kvs__multi_process
   :failure_id: UI_01_09
   :failure_effect: Unsynchronized access of two processes makes the persisted data inconsistent.
   :safety_relevant: yes
   :mitigated_by: comp_req__kvs__single_process_access
   :sufficient: yes
   :status: valid
   :version: 1

   Implementation: only in-process mutexes are used.

.. comp_saf_dfa:: Memory depletion by the kvs
   :violates: comp_arc_sta__persistency_kvs__sv
   :id: comp_saf_dfa__kvs__memory_depletion
   :failure_id: UI_01_11
   :failure_effect: Runtime allocation of the kvs depletes the heap shared with other software elements of the process.
   :safety_relevant: yes
   :mitigated_by: comp_req__kvs__no_dynamic_memory, comp_req__kvs__constraints
   :sufficient: yes
   :status: valid
   :version: 1

   Implementation: both implementations allocate at runtime.

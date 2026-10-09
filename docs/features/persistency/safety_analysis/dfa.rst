..
   # *******************************************************************************
   # Copyright (c) 2024 Contributors to the Eclipse Foundation
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

DFA (Dependent Failure Analysis)
################################

.. document:: Persistency DFA
   :id: doc__persistency_dfa
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__feature_dfa[version==1]
   :tags: persistency


The DFA applies the dependent failure initiators of :need:`gd_guidl__dfa_failure_initiators` to the static view of
:need:`doc__persistency_kvs_architecture`.

Dependent Failure Initiators
----------------------------

Shared resources
^^^^^^^^^^^^^^^^

The dependent failure initiators related to shared resources are not applicable for the features. The shared resources will be considered in the platform DFA.

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
    - Only function arguments and return values within the calling thread; no shared globals. Corruption: see CO_01_02.
  * - CO_01_02
    - Data or message corruption / repetition / loss / delay / masquerading or incorrect addressing of information
    - yes
    - :need:`feat_saf_dfa__persistency__data_corruption`
  * - CO_01_03
    - Insertion / sequence of information
    - no
    - Data and hash files are written and read as a whole; see CO_01_02.
  * - CO_01_04
    - Corruption of information, inconsistent data
    - yes
    - Covered by :need:`feat_saf_dfa__persistency__data_corruption`.
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
    - No communication channel used; blocking of the caller: see UI_01_06.


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
    - Each instance uses its own data, hash and default value files (instance ID). Shared instances: see SI_01_03.
  * - SI_01_03
    - Constants, or variables, being global to the two software functions
    - yes
    - :need:`feat_saf_dfa__persistency__shared_instance`
  * - SI_01_04
    - Basic software passes data (read from hardware register and converted into logical information) to two applications software functions
    - no
    - No hardware registers are read; file access via the OS.
  * - SI_01_05
    - Data / function parameter arguments / messages delivered by software function to more than one other function
    - no
    - Values are returned to the calling function only.


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
    - Only own instance memory and caller memory is accessed. Access by others is prevented by OS process isolation (platform DFA).
  * - UI_01_03
    - Stack/Buffer under-/overflow
    - no
    - Rust bounds checking, C++ standard containers. Overflows by others: platform DFA.
  * - UI_01_04
    - Deadlocks
    - no
    - One mutex per instance (Rust additionally one for the instance pool in the builder); never more than one lock is held at a time. Rust waits for the lock; C++ does not wait and returns MutexLockFailed.
  * - UI_01_05
    - Livelocks
    - no
    - Same as UI_01_04; no retry loops.
  * - UI_01_06
    - Blocking of execution
    - yes
    - :need:`feat_saf_dfa__persistency__execution_blocking`
  * - UI_01_07
    - Incorrect allocation of execution time
    - no
    - No own execution context; see UI_01_06.
  * - UI_01_08
    - Incorrect execution flow
    - no
    - Execution flow defined by the dynamic views; corruption: see UI_01_02.
  * - UI_01_09
    - Incorrect synchronization between software elements
    - yes
    - :need:`feat_saf_dfa__persistency__multi_process`
  * - UI_01_10
    - CPU time depletion
    - no
    - No own execution context; see UI_01_06.
  * - UI_01_11
    - Memory depletion
    - yes
    - :need:`feat_saf_dfa__persistency__memory_depletion`
  * - UI_01_12
    - Other HW unavailability
    - no
    - Storage medium only, via file system; unavailability is reported and handled by aou_req__persistency__error_handling.


DFA
---

.. feat_saf_dfa:: Persistency execution blocking
   :violates: feat_arc_sta__persistency__static
   :id: feat_saf_dfa__persistency__execution_blocking
   :failure_id: UI_01_06
   :failure_effect: Blocking of execution, persistency is not available.
   :safety_relevant: yes
   :mitigated_by: aou_req__persistency__error_handling
   :sufficient: yes
   :status: valid
   :version: 2

   Persistency runs in the calling context; blocking (by the application or OS scheduling) leads to no or a
   too late response, handled by :need:`aou_req__persistency__error_handling`.

.. feat_saf_dfa:: Corruption of persisted data
   :violates: feat_arc_sta__persistency__static
   :id: feat_saf_dfa__persistency__data_corruption
   :failure_id: CO_01_02
   :failure_effect: Data exchanged with the file system is corrupted, lost or inconsistent.
   :safety_relevant: yes
   :mitigated_by: feat_req__persistency__integrity_check, feat_req__persistency__reset_resistant, feat_req__persistency__access_control, aou_req__persistency__error_handling
   :sufficient: yes
   :status: valid
   :version: 1

   Causes: storage medium, interrupted write, other software writing the files. Detected by the hash check
   (:need:`feat_req__persistency__integrity_check`), writes are reset resistant
   (:need:`feat_req__persistency__reset_resistant`), access by others is prevented by
   :need:`feat_req__persistency__access_control`. Reported errors: :need:`aou_req__persistency__error_handling`.

.. feat_saf_dfa:: Shared KVS instance within a process
   :violates: feat_arc_sta__persistency__static
   :id: feat_saf_dfa__persistency__shared_instance
   :failure_id: SI_01_03
   :failure_effect: Two software elements of a process use the same KVS instance and modify each other's data.
   :safety_relevant: yes
   :mitigated_by: aou_req__persistency__instance_separation
   :sufficient: yes
   :status: valid
   :version: 1

   Within a process, the same instance ID gives access to the same data. Mitigated by
   :need:`aou_req__persistency__instance_separation`.

.. feat_saf_dfa:: Access to a KVS instance from multiple processes
   :violates: feat_arc_sta__persistency__static
   :id: feat_saf_dfa__persistency__multi_process
   :failure_id: UI_01_09
   :failure_effect: Unsynchronized access of two processes makes the persisted data inconsistent.
   :safety_relevant: yes
   :mitigated_by: feat_req__persistency__multiple_app
   :sufficient: yes
   :status: valid
   :version: 1

   Synchronization covers threads of one process only. Access from multiple processes is prevented by
   :need:`feat_req__persistency__multiple_app`.

   Note: Not yet implemented (only in-process mutexes; component level).

.. feat_saf_dfa:: Memory depletion by the kvs
   :violates: feat_arc_sta__persistency__static
   :id: feat_saf_dfa__persistency__memory_depletion
   :failure_id: UI_01_11
   :failure_effect: Runtime allocation of the kvs depletes the heap shared with other software elements.
   :safety_relevant: yes
   :mitigated_by: feat_req__persistency__dynamic_memory_alloc, feat_req__persistency__cfg
   :sufficient: yes
   :status: valid
   :version: 1

   All memory shall be allocated at initialization (:need:`feat_req__persistency__dynamic_memory_alloc`),
   bounded by the configured storage and key size (:need:`feat_req__persistency__cfg`).

   Note: The implementations still allocate at runtime (component level).

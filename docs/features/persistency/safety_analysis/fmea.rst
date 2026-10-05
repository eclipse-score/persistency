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

Persistency Failure Modes and Effects Analysis
##############################################

.. document:: Persistency FMEA
   :id: doc__persistency_fmea
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__feature_fmea[version==1]
   :tags: persistency


The FMEA applies the fault models of :need:`gd_guidl__fault_models` to the dynamic views of
:need:`doc__persistency_kvs_architecture`.

Failure Mode List
-----------------

.. list-table:: Fault Models for sequence diagrams
    :header-rows: 1
    :widths: 10,20,10,20

    * - ID
      - Failure Mode
      - Applicability
      - Rationale
    * - MF_01_01
      - message is not received (is a subset/more precise description of MF_01_05)
      - yes
      - :need:`feat_saf_fmea__persistency__msg_not_received`
    * - MF_01_02
      - message received too late (only relevant if delay is a realistic fault)
      - yes
      - :need:`feat_saf_fmea__persistency__late_message`
    * - MF_01_03
      - message received too early (usually not a problem)
      - no
      - Synchronous calls with response; an early response is not possible.
    * - MF_01_04
      - message not received correctly by all recipients (different messages or messages partly lost). Only relevant if the same message goes to multiple recipients.
      - no
      - Every message has exactly one recipient.
    * - MF_01_05
      - message is corrupted
      - yes
      - :need:`feat_saf_fmea__persistency__corrupted_message`, :need:`feat_saf_fmea__persistency__corrupt_load`, :need:`feat_saf_fmea__persistency__corrupt_store`
    * - MF_01_06
      - message is not sent
      - yes
      - :need:`feat_saf_fmea__persistency__not_sent`
    * - MF_01_07
      - message is unintended sent
      - no
      - Messages to json and filesystem are only sent on a user request; data is only written on explicit flush.
    * - CO_01_01
      - minimum constraint boundary is violated
      - no
      - No minimum constraints are defined (snapshot max count 0 is valid and disables snapshots).
    * - CO_01_02
      - maximum constraint boundary is violated
      - yes
      - :need:`feat_saf_fmea__persistency__constraint_max`
    * - EX_01_01
      - Process calculates wrong result(s) (is a subset/more precise description of MF_01_05 or MF_01_04). This failure mode is related to the analysis if e.g. internal safety mechanisms are required (level 2 function, plausibility check of the output, …) because of the size / complexity of the feature.
      - no
      - No calculations on values, only storing and (de)serialization. Systematic faults are addressed by ASIL B development and verification; corruption is covered by MF_01_05.
    * - EX_01_02
      - processing too slow (only relevant if timing is considered)
      - yes
      - Covered by :need:`feat_saf_fmea__persistency__late_message`.
    * - EX_01_03
      - processing too fast (only relevant if timing is considered)
      - no
      - No minimum processing time required; no hazardous effect.
    * - EX_01_04
      - loss of execution
      - yes
      - :need:`feat_saf_fmea__persistency__err_handl`
    * - EX_01_05
      - processing changes to arbitrary process
      - no
      - The kvs runs in the calling thread. Control flow changes by HW faults or memory corruption are covered by the platform DFA and the environment assumptions.
    * - EX_01_06
      - processing is not complete (infinite loop)
      - no
      - All loops iterate over finite data (keys, file content, snapshots); parsing ends with the input.

FMEA
----

.. feat_saf_fmea:: Message is not received
    :violates: feat_arc_dyn__persistency__check_key_default, feat_arc_dyn__persistency__delete_key, feat_arc_dyn__persistency__flush, feat_arc_dyn__persistency__read_key, feat_arc_dyn__persistency__read_from_storage, feat_arc_dyn__persistency__write_key, feat_arc_dyn__persistency__snapshot_restore
    :id: feat_saf_fmea__persistency__msg_not_received
    :fault_id: MF_01_01
    :failure_effect: Request or response is not received, the operation is not performed or its result is not available.
    :failure_root_cause: The call does not return, e.g. due to a blocked execution context or a file system access that does not complete.
    :safety_relevant: yes
    :mitigated_by: aou_req__persistency__error_handling
    :sufficient: yes
    :status: valid
    :version: 2

    Applies to all messages between user and kvs and to the responses of json and filesystem. The effect is
    an unavailability of the persistency (call does not return), which the application detects and handles
    (:need:`aou_req__persistency__error_handling`). No wrong data is used.

.. feat_saf_fmea:: Message received too late
    :violates: feat_arc_dyn__persistency__check_key_default, feat_arc_dyn__persistency__delete_key, feat_arc_dyn__persistency__flush, feat_arc_dyn__persistency__read_key, feat_arc_dyn__persistency__read_from_storage, feat_arc_dyn__persistency__write_key, feat_arc_dyn__persistency__snapshot_restore
    :id: feat_saf_fmea__persistency__late_message
    :fault_id: MF_01_02
    :failure_effect: The response is received too late.
    :failure_root_cause: Long file system access times or delayed scheduling of the calling execution context.
    :safety_relevant: yes
    :mitigated_by: aou_req__persistency__error_handling
    :sufficient: yes
    :status: valid
    :version: 2

    Relevant for operations accessing the file system (load, flush, snapshot restore). Same effect as
    MF_01_01, handled by :need:`aou_req__persistency__error_handling`. :need:`feat_req__persistency__async_api`
    avoids blocking for time consuming operations.

.. feat_saf_fmea:: Message is corrupted between user and kvs
    :violates: feat_arc_dyn__persistency__check_key_default, feat_arc_dyn__persistency__delete_key, feat_arc_dyn__persistency__read_key, feat_arc_dyn__persistency__write_key
    :id: feat_saf_fmea__persistency__corrupted_message
    :fault_id: MF_01_05
    :failure_effect: A key or value passed between user and kvs is corrupted.
    :failure_root_cause: Random hardware fault or memory corruption by another software element of the same process.
    :safety_relevant: yes
    :mitigated_by: aou_req__persistency__error_handling
    :sufficient: yes
    :status: valid
    :version: 2

    The messages are function calls within the same process. Corruption is only possible by random HW faults
    (environment assumptions) or memory corruption by other software elements (:need:`doc__persistency_dfa`).
    Reported errors are handled by :need:`aou_req__persistency__error_handling`.

.. feat_saf_fmea:: Persisted data is corrupted when loading
    :violates: feat_arc_dyn__persistency__read_from_storage, feat_arc_dyn__persistency__snapshot_restore
    :id: feat_saf_fmea__persistency__corrupt_load
    :fault_id: MF_01_05
    :failure_effect: Corrupted data or hash file content is read from the file system.
    :failure_root_cause: Fault of the storage medium, interrupted write operation or modification of the files by another software element.
    :safety_relevant: yes
    :mitigated_by: feat_req__persistency__integrity_check, feat_req__persistency__snapshot_restore, aou_req__persistency__error_handling
    :sufficient: yes
    :status: valid
    :version: 2

    The kvs checks the hash before parsing (:need:`feat_req__persistency__integrity_check`) and reports
    Validation-Failed-Error; invalid JSON is reported as JSON-Parser-Error. Corrupted data is never provided.
    The application handles the error (:need:`aou_req__persistency__error_handling`), e.g. by restoring a
    snapshot (:need:`feat_req__persistency__snapshot_restore`).

.. feat_saf_fmea:: Persisted data is corrupted when storing
    :violates: feat_arc_dyn__persistency__flush
    :id: feat_saf_fmea__persistency__corrupt_store
    :fault_id: MF_01_05
    :failure_effect: Data or hash file is written corrupted or incomplete (e.g. interrupted write).
    :failure_root_cause: Reset or power loss during the write operation, or fault of the storage medium.
    :safety_relevant: yes
    :mitigated_by: feat_req__persistency__reset_resistant, feat_req__persistency__recovery_from_reset, feat_req__persistency__integrity_check, aou_req__persistency__error_handling
    :sufficient: yes
    :status: valid
    :version: 2

    Write operations are reset resistant (:need:`feat_req__persistency__reset_resistant`,
    :need:`feat_req__persistency__recovery_from_reset`). An inconsistent data/hash pair is detected at the next load
    (:need:`feat_saf_fmea__persistency__corrupt_load`). Write errors are reported as Physical-Storage-Failure.

.. feat_saf_fmea:: Message is not sent
    :violates: feat_arc_dyn__persistency__check_key_default, feat_arc_dyn__persistency__delete_key, feat_arc_dyn__persistency__flush, feat_arc_dyn__persistency__read_key, feat_arc_dyn__persistency__read_from_storage, feat_arc_dyn__persistency__write_key, feat_arc_dyn__persistency__snapshot_restore
    :id: feat_saf_fmea__persistency__not_sent
    :fault_id: MF_01_06
    :failure_effect: A response or write request is not sent.
    :failure_root_cause: Systematic fault in the kvs or in the used baselibs interfaces, or failed file system operation.
    :safety_relevant: yes
    :mitigated_by: aou_req__persistency__error_handling
    :sufficient: yes
    :status: valid
    :version: 2

    Same effect as :need:`feat_saf_fmea__persistency__msg_not_received`. A missing write in the flush view is
    reported as Physical-Storage-Failure.

.. feat_saf_fmea:: Maximum constraint boundary is violated
    :violates: feat_arc_dyn__persistency__write_key, feat_arc_dyn__persistency__flush, feat_arc_dyn__persistency__read_from_storage
    :id: feat_saf_fmea__persistency__constraint_max
    :fault_id: CO_01_02
    :failure_effect: A key, value or the persisted data exceeds the configured maximum.
    :failure_root_cause: The application stores keys, values or data exceeding the configured limits.
    :safety_relevant: yes
    :mitigated_by: feat_req__persistency__cfg, aou_req__persistency__error_handling
    :sufficient: yes
    :status: valid
    :version: 2

    Limits are configured according to :need:`feat_req__persistency__cfg`. Exceeding the storage results in
    Physical-Storage-Failure; persisted data and snapshots remain unchanged.

    Note: :need:`comp_req__kvs__key_length` and :need:`comp_req__kvs__value_length` are not yet enforced by the
    implementation (component level).

.. feat_saf_fmea:: Loss of execution
    :violates: feat_arc_dyn__persistency__check_key_default, feat_arc_dyn__persistency__delete_key, feat_arc_dyn__persistency__flush, feat_arc_dyn__persistency__read_key, feat_arc_dyn__persistency__read_from_storage, feat_arc_dyn__persistency__write_key, feat_arc_dyn__persistency__snapshot_restore
    :id: feat_saf_fmea__persistency__err_handl
    :fault_id: EX_01_04
    :failure_effect: The persistency is not available.
    :failure_root_cause: Termination of the calling thread or process.
    :safety_relevant: yes
    :mitigated_by: aou_req__persistency__error_handling
    :sufficient: yes
    :status: valid
    :version: 2

    The kvs runs in the calling context, so a loss of execution stops the application itself. An interrupted
    flush is covered by :need:`feat_saf_fmea__persistency__corrupt_store`. Handled by
    :need:`aou_req__persistency__error_handling`.

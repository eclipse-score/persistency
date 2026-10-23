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


KVS FMEA (Failure Modes and Effects Analysis)
=============================================

.. document:: KVS FMEA
   :id: doc__kvs_fmea
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_component_fmea[version==1]
   :tags: Persistency KVS

Failure modes at the component interfaces are analysed in :need:`doc__persistency_fmea`. This FMEA applies the
fault models of :need:`gd_guidl__fault_models` to the mechanisms specified in :need:`doc__kvs_requirements`.
The scope is defined in :need:`doc__persistency_safety_plan` (tailoring).

Failure Mode List
-----------------

.. list-table:: Fault Models for the component mechanisms
    :header-rows: 1
    :widths: 10,20,10,20

    * - ID
      - Failure Mode
      - Applicability
      - Rationale
    * - MF_01_01
      - message is not received (is a subset/more precise description of MF_01_05)
      - no
      - See :need:`feat_saf_fmea__persistency__msg_not_received`.
    * - MF_01_02
      - message received too late (only relevant if delay is a realistic fault)
      - no
      - See :need:`feat_saf_fmea__persistency__late_message`.
    * - MF_01_03
      - message received too early (usually not a problem)
      - no
      - Synchronous calls; an early response is not possible.
    * - MF_01_04
      - message not received correctly by all recipients (different messages or messages partly lost). Only relevant if the same message goes to multiple recipients.
      - no
      - Every message has exactly one recipient.
    * - MF_01_05
      - message is corrupted
      - yes
      - :need:`comp_saf_fmea__kvs__flush_interrupted`, :need:`comp_saf_fmea__kvs__corrupt_on_open`, :need:`comp_saf_fmea__kvs__default_corrupt`
    * - MF_01_06
      - message is not sent
      - yes
      - :need:`comp_saf_fmea__kvs__flush_skipped`
    * - MF_01_07
      - message is unintended sent
      - no
      - Storage is only written by flush, snapshots only changed on explicit call (:need:`comp_req__kvs__explicit_flush`, :need:`comp_req__kvs__explicit_snapshot_operations`). Implementation: flush rotates the snapshots.
    * - CO_01_01
      - minimum constraint boundary is violated
      - yes
      - :need:`comp_saf_fmea__kvs__constraint_exceeded`
    * - CO_01_02
      - maximum constraint boundary is violated
      - yes
      - :need:`comp_saf_fmea__kvs__constraint_exceeded`, :need:`comp_saf_fmea__kvs__constraint_cfg_wrong`
    * - EX_01_01
      - Process calculates wrong result(s) (is a subset/more precise description of MF_01_05 or MF_01_04). This failure mode is related to the analysis if e.g. internal safety mechanisms are required (level 2 function, plausibility check of the output, …) because of the size / complexity of the feature.
      - yes
      - :need:`comp_saf_fmea__kvs__empty_storage_used`, :need:`comp_saf_fmea__kvs__migration_wrong`, :need:`comp_saf_fmea__kvs__tool_wrong_output`
    * - EX_01_02
      - processing too slow (only relevant if timing is considered)
      - no
      - See :need:`feat_saf_fmea__persistency__late_message`.
    * - EX_01_03
      - processing too fast (only relevant if timing is considered)
      - no
      - No minimum processing time required.
    * - EX_01_04
      - loss of execution
      - yes
      - :need:`comp_saf_fmea__kvs__unflushed_lost`
    * - EX_01_05
      - processing changes to arbitrary process
      - no
      - Runs in the calling thread; control flow corruption: platform DFA.
    * - EX_01_06
      - processing is not complete (infinite loop)
      - no
      - All loops iterate over finite data.

FMEA
----

.. comp_saf_fmea:: Flush is interrupted
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__flush_interrupted
    :fault_id: MF_01_05
    :failure_effect: Data and hash file are written incompletely.
    :failure_root_cause: Reset or power loss during the flush, write error of the storage medium.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__reset_resistant_write, comp_req__kvs__atomic_store, comp_req__kvs__pers_data_csum, comp_req__kvs__corruption_detect_open, comp_req__kvs__recovery_snapshot_restore
    :sufficient: yes
    :status: valid
    :version: 1

    The previous state is kept until the flush is complete; an incomplete flush is detected at the next open.

    Implementation: the previous state is only kept by the snapshot rotation, so with a maximum snapshot count of
    ``0`` :need:`comp_req__kvs__reset_resistant_write` is not met.

.. comp_saf_fmea:: Corrupted storage at open, reload or snapshot restore
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__corrupt_on_open
    :fault_id: MF_01_05
    :failure_effect: Corrupted or inconsistent data is loaded into the in-memory cache.
    :failure_root_cause: Interrupted flush, fault of the storage medium, data or hash file missing or modified by another software element.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__pers_data_csum_vrfy, comp_req__kvs__corruption_detect_open, comp_req__kvs__open_need_kvs_behavior, comp_req__kvs__key_uniqueness, comp_req__kvs__permission_err_hndl
    :sufficient: yes
    :status: valid
    :version: 1

    Applies to every load: open, :need:`comp_req__kvs__discard_pending_changes_api` and snapshot restore.

    Implementation (Rust): a missing hash file is reported as ``FileNotFound``, so with ``Optional`` the instance
    is opened with empty storage although the data file exists.

.. comp_saf_fmea:: Empty storage is used instead of persisted data
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__empty_storage_used
    :fault_id: EX_01_01
    :failure_effect: The application reads default values instead of the persisted values without an error being reported.
    :failure_root_cause: Open with ``Ignore``, or with ``Optional`` while no storage file exists (e.g. wrong storage path).
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__open_need_kvs_behavior, comp_req__kvs__key_exists_api, comp_req__kvs__default_value_is_default_api, aou_req__persistency__start_without_data
    :sufficient: yes
    :status: valid
    :version: 1

    The application selects the open behavior and can detect values which are not persisted.

.. comp_saf_fmea:: Default value file is corrupted or modified
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__default_corrupt
    :fault_id: MF_01_05
    :failure_effect: Wrong default values are provided to the application.
    :failure_root_cause: Fault of the storage medium, modification of the default value file after deployment.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__default_val_chksum, comp_req__kvs__default_storage_permissions, comp_req__kvs__default_value_cfg_tool
    :sufficient: yes
    :status: valid
    :version: 1

    Implementation: the kvs tool does not set the file permissions.

.. comp_saf_fmea:: Flush does not write changed data
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__flush_skipped
    :fault_id: MF_01_06
    :failure_effect: Flush returns without error, but the changes are lost at the next reset.
    :failure_root_cause: The change detection of :need:`comp_req__kvs__write_amplification` misses a change.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__write_amplification, comp_req__kvs__explicit_flush
    :sufficient: no
    :status: valid
    :version: 1

    A missed change is not detected by the checksum, which covers the written data only. Not sufficient: it is not
    specified that every modifying operation marks the data as changed.

    Implementation: no change detection, flush always writes.

.. comp_saf_fmea:: Constraint boundary is violated
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__constraint_exceeded
    :fault_id: CO_01_02
    :failure_effect: A key, the snapshots or the persisted data exceed the configured limits, or an invalid snapshot slot is used.
    :failure_root_cause: The application passes an invalid key or snapshot slot index, or stores more data than configured.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__key_naming, comp_req__kvs__key_length, comp_req__kvs__constraints, comp_req__kvs__no_dynamic_memory, comp_req__kvs__snapshot_max_num_cfg, comp_req__kvs__snapshot_slot_index_vali, comp_req__kvs__snapshot_restore_api, comp_req__kvs__snapshot_remove_api
    :sufficient: no
    :status: valid
    :version: 1

    Not sufficient: :need:`comp_req__kvs__snapshot_slot_index_vali` checks the fixed range of
    :need:`comp_req__kvs__snapshot_id_api`, not the configured maximum of :need:`comp_req__kvs__snapshot_max_num_cfg`.
    A snapshot above the configured maximum exceeds the configured storage.

    Implementation: Rust does not validate keys; the snapshot create, remove and slot query API is missing.

.. comp_saf_fmea:: Wrong constraint configuration file
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__constraint_cfg_wrong
    :fault_id: CO_01_02
    :failure_effect: The instance operates with wrong limits, e.g. snapshots disabled or limits above the dimensioned memory.
    :failure_root_cause: Corrupted, modified or wrong configuration file.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__constraints, comp_req__kvs__constraints_priority
    :sufficient: no
    :status: valid
    :version: 1

    The file overrides the compile-time values (:need:`comp_req__kvs__constraints_priority`). Not sufficient: no
    integrity or plausibility check of the file is specified.

.. comp_saf_fmea:: Data migration produces wrong data
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__migration_wrong
    :fault_id: EX_01_01
    :failure_effect: Migrated values differ from the original values and are persisted at the next flush.
    :failure_root_cause: Systematic fault in the migration, unsupported schema version.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__pers_data_version, comp_req__kvs__pers_data_schema, comp_req__kvs__pers_data_migration
    :sufficient: no
    :status: valid
    :version: 1

    The checksum does not cover the correctness of the migration. Not sufficient: it is not specified that an
    unsupported schema version is reported and that the original data is kept until the migrated data is flushed.

    Implementation: schema version and migration are missing.

.. comp_saf_fmea:: Changes are lost by loss of execution
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__unflushed_lost
    :fault_id: EX_01_04
    :failure_effect: Changes which are not flushed are lost; after restart the last flushed state is provided.
    :failure_root_cause: Termination of the calling process or reset before the flush.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__explicit_flush, comp_req__kvs__cached_access, aou_req__persistency__flush
    :sufficient: yes
    :status: valid
    :version: 1

    Interrupted flush: :need:`comp_saf_fmea__kvs__flush_interrupted`.

.. comp_saf_fmea:: Kvs tool generates wrong storage or default value files
    :violates: comp_arc_sta__persistency_kvs__sv
    :id: comp_saf_fmea__kvs__tool_wrong_output
    :fault_id: EX_01_01
    :failure_effect: Wrong default or persisted values are provided without an error being reported.
    :failure_root_cause: Systematic fault of the kvs tool.
    :safety_relevant: yes
    :mitigated_by: comp_req__kvs__default_value_cfg_tool, comp_req__kvs__storage_tool, comp_req__kvs__default_val_chksum, comp_req__kvs__pers_data_csum
    :sufficient: no
    :status: valid
    :version: 1

    The tool computes the checksum over its own output, so wrong content is not detected at loading. Not
    sufficient: neither a tool qualification nor a verification of the generated files is specified.

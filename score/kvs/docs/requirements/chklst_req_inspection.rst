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


.. document:: KVS Requirements Inspection Checklist
   :id: doc__kvs_req_inspection
   :status: draft
   :version: 2
   :safety: ASIL_B
   :security: YES
   :realizes: wp__requirements_inspect[version==1]

Requirement Inspection Checklist
================================

Purpose
-------

The purpose of this requirement inspection checklist is to collect the topics to be checked during requirements inspection.

Conduct
-------

As described in the concept :need:`doc_concept__wp_inspections` the following "inspection roles" are expected to be filled:

- content responsible (author): `<https://github.com/Ahmed-Elsaka-JC>`_
- reviewer: `<https://github.com/aschemmel-tech>`_, `<https://github.com/umaucher>`_
- moderator: `<https://github.com/PandaeDo>`_
- test expert: `<https://github.com/PiotrKorkus>`_

Checklist
---------

It is mandatory to fill in the "passed" column with "yes" or "no" for each checklist item and additionally to add in the remarks why it is passed or not passed.
In case of "no" an issue link to the issue tracking system has to be added in the last column (if not solved in the same issue).
See also :need:`doc_concept__wp_inspections` for further information about reviews in general and inspection in particular.

.. note::

   **Pre-check for the re-inspection (version 2)**

   The columns "Passed", "Remarks" and "Issue link" contain a pre-check prepared by the safety manager as a basis
   for discussion. "Proposal: YES/NO" is not an inspection result; the inspection participants confirm or correct
   each entry, the moderator replaces the proposals by the final results.

   The pre-check was performed against the component requirements after their update with
   `#443 <https://github.com/eclipse-score/persistency/pull/443>`_ and the AoUs listed below, the feature
   requirements of the Persistency feature and the C++/Rust implementation. Findings of version 1
   (`#297 <https://github.com/eclipse-score/persistency/issues/297>`_) are marked as resolved or still open.

   To be decided by the moderator before the inspection: the scope tables below select all component requirements
   in "valid" state (65) independent of the "inspected" tag, while the text restricts the scope to requirements with
   the "inspected" tag. The tag is currently set inconsistently (``inspected`` on 12, ``valid`` on 7 requirements).

.. list-table:: Component Requirement Inspection Checklist
    :header-rows: 1
    :widths: 10,30,50,6,6,8

    * - Review ID
      - Acceptance Criteria
      - Guidance
      - Passed
      - Remarks
      - Issue link
    * - REQ_01_01
      - Is the requirement formulation template used?
      - see :need:`gd_temp__req_formulation`, this includes the use of "shall".
      - Proposal: YES
      - - All component requirements use "The component shall ...", both AoUs use "The user shall ...".
        - Minor: :need:`comp_req__kvs__snapshot_source` lacks the condition ("when creating a snapshot"); second sentence of :need:`comp_req__kvs__no_dynamic_memory` has no subject ("All memory ... shall be allocated").
      -
    * - REQ_02_01
      - Is the requirement description *comprehensible* ?
      - If you think the requirement is hard to understand, comment here.
      - Proposal: NO
      - - Findings of version 1 resolved: "snapshot" is explained in the note of :need:`comp_req__kvs__snapshot_create_api`; "only permitted" removed from :need:`comp_req__kvs__default_value_types`; :need:`comp_req__kvs__pers_data_schema` no longer mentions downgrade.
        - :need:`comp_req__kvs__snapshot_source`: not understandable stand-alone - it is not stated for which operation the live values are used.
        - :need:`comp_req__kvs__callback_support`: "data change events" are not defined (which changes, which trigger point: set, flush, restore?).
      - new issue to be created after inspection
    * - REQ_02_02
      - Is the requirement description *unambiguous* ?
      - Especially search for "weak words" like "about", "etc.", "relevant" and others (see the internet documentation on this). This check shall be supported by tooling.
      - Proposal: NO
      - - :need:`comp_req__kvs__value_data_types`: "Number" does not define range/precision (implementation offers i32, u32, i64, u64, f64).
        - :need:`comp_req__kvs__constraints`: "KVS constraints" are not enumerated (e.g. storage size, max. number of snapshots), although :need:`comp_req__kvs__no_dynamic_memory` relies on them.
        - :need:`comp_req__kvs__os_abstraction`: it is not named which operating system abstraction layer is meant.
        - :need:`comp_req__kvs__storage_tool`: "validate" - against what (checksum, schema, constraints)?
      - new issue to be created after inspection
    * - REQ_02_03
      - Is the requirement description *atomic* ?
      - A good way to think about this is to consider if the requirement may be tested by one (positive) test case or needs more of these. The requirement formulation template should also avoid being non-atomic already. Note that there are cases where also non-atomic requirements are the better ones, for example if those are better understandable.
      - Proposal: YES
      - - :need:`comp_req__kvs__storage_tool` combines three capabilities (view, modify, validate) - discuss splitting.
        - :need:`comp_req__kvs__open_need_kvs_behavior` contains three cases; acceptable as they describe one configuration parameter.
      -
    * - REQ_02_04
      - Is the requirement description *feasible* ?
      - If at the time of the inspection the requirement has already some implementation, the answer is yes. This can be checked via traces, but also :need:`gd_req__req_attr_impl` shows this. In case the requirement has no implementation at the time of inspection (i.e. not implemented at least as "proof-of-concept"), a development expert should be invited to the Pull-Request review to explicitly check this item.
      - Proposal: NO
      - - Several requirements have no implementation yet; a development expert shall confirm feasibility:
        - :need:`comp_req__kvs__async_api`, :need:`comp_req__kvs__callback_support` - no asynchronous API and no callback registration in C++/Rust API.
        - :need:`comp_req__kvs__snapshot_create_api`, :need:`comp_req__kvs__snapshot_create`, :need:`comp_req__kvs__snapshot_overwrite`, :need:`comp_req__kvs__snapshot_slot_free_api`, :need:`comp_req__kvs__snapshot_remove_api`, :need:`comp_req__kvs__explicit_snapshot_operations` - implementation creates snapshots implicitly by rotation during flush (``kvs.cpp`` ``SnapshotRotate``, ``json_backend.rs`` ``snapshot_rotate``), no slot-based create/remove/free API.
        - :need:`comp_req__kvs__single_process_access` - no inter-process locking found in the implementation.
        - :need:`comp_req__kvs__pers_data_version`, :need:`comp_req__kvs__pers_data_schema`, :need:`comp_req__kvs__pers_data_migration` - no schema version field and no schema migration (``examples/migration.rs`` migrates between backends).
        - :need:`comp_req__kvs__no_dynamic_memory` - implementation uses heap-based containers for key-value pairs after open; feasibility to be confirmed.
      - new issue to be created after inspection
    * - REQ_02_05
      - Is the requirement description *independent from implementation* ?
      - This checkpoint should improve requirements definition in the sense that the "what" is described and not the "how" - the latter should be described in architecture/design derived from the requirement. But there can also be a good reason for this, for example we would require using a file format like JSON and even specify the formatting standard already on stakeholder requirement level because we want to be compatible. A finding in this checkpoint does not mean there is a safety problem in the requirement.
      - Proposal: YES
      - Component level; implementation details like file permissions (:need:`comp_req__kvs__storage_file_permissions`), JSON format (:need:`comp_req__kvs__pers_data_store_fmt`) and fixed API name ``get_default_value`` (:need:`comp_req__kvs__value_default`) are acceptable or justified by compatibility - to be confirmed.
      -
    * - REQ_03_01
      - Is the *linkage to the parent requirement* correct?
      - Linkage to correct levels and ASIL attributes is checked automatically, but it needs checking if the child requirement implements (at least) a part of the parent requirement.
      - Proposal: NO
      - - Findings of version 1 resolved: :need:`comp_req__kvs__default_value_query`, :need:`comp_req__kvs__default_value_cfg` and :need:`comp_req__kvs__default_val_chksum` now link to fitting parents.
        - :need:`comp_req__kvs__callback_support` (data change callbacks) does not implement :need:`feat_req__persistency__async_completion` (completion signalling of asynchronous operations).
        - :need:`comp_req__kvs__pers_data_store_bnd` and :need:`comp_req__kvs__pers_data_store_fmt` do not implement a part of :need:`feat_req__persistency__integrity_check`.
        - :need:`comp_req__kvs__key_value_overwrite` is linked to :need:`feat_req__persistency__support_datatype_keys` - better :need:`feat_req__persistency__support_datatype_value` or :need:`feat_req__persistency__cached_access`.
      - new issue to be created after inspection
    * - REQ_04_01
      - Is the requirement *internally and externally consistent*?
      - Does the requirement contradict other requirements within the same or higher levels? One may restrict the search to the feature for component requirements, for features to other features using same components. Is the description of the requirement consistent with all its attributes (if not already part of another check, e.g. does the title fit?).
      - Proposal: NO
      - - Finding of version 1 on IDs resolved (IDs now ``comp_req__kvs__...``).
        - Still open from version 1: :need:`comp_req__kvs__value_serialize` fixes serialization to JSON, which contradicts selectable storage backends (:need:`comp_req__kvs__storage_backend_select`); also :need:`comp_req__kvs__pers_data_store_fmt` fixes JSON for all backends.
        - :need:`comp_req__kvs__value_default` (default value "when no value has been set") overlaps / contradicts :need:`comp_req__kvs__default_value_query` (default value "even if value is set").
        - :need:`comp_req__kvs__snapshot_slot_index_vali` validates against the fixed range ``<1..3>``, not against the configured maximum of :need:`comp_req__kvs__snapshot_max_num_cfg` (e.g. max. = 1 and index = 2).
        - :need:`feat_req__persistency__snapshot_create` requires snapshots identified by unique IDs; the component requirements use reusable slot indices.
        - :need:`aou_req__kvs__limits` requires the user to configure the maximum storage size, but no component requirement provides this configuration parameter.
        - Attribute ``tags`` is used inconsistently (``inspected`` on 12, ``valid`` on 7 requirements) - see note above the checklist.
      - new issue to be created after inspection
    * - REQ_05_01
      - Do the software requirements consider *timing constraints*?
      - This checkpoint encourages to think about timing constraints even if those are not explicitly mentioned in the parent requirement. If the reviewer of a requirement already knows or suspects that the code execution will be consuming a lot of time, one should think of the expectation of a "user".
      - Proposal: YES
      - - Bounded execution time is addressed by :need:`aou_req__kvs__filesystem`, :need:`aou_req__kvs__limits` and :need:`comp_req__kvs__no_dynamic_memory`.
        - Discuss: no explicit timing requirement for ``open``/``flush``/snapshot restore (e.g. worst-case execution time relative to storage size).
      -
    * - REQ_06_01
      - Does the requirement consider *external interfaces*?
      - The SW platform's external interfaces (to the user) are defined in the Feature Architecture, so the Feature and Component Requirements should determine the input data use and setting of output data for these interfaces. Are all output values defined?
      - Proposal: NO
      - - Improved since version 1 (open behavior: :need:`comp_req__kvs__open_need_kvs_behavior`, flush: :need:`comp_req__kvs__explicit_flush`, :need:`comp_req__kvs__discard_pending_changes_api`).
        - Output values / error reporting not defined for: reading a key without value and without default, invalid keys (:need:`comp_req__kvs__key_length`, :need:`comp_req__kvs__key_naming` say "disallow"/"accept" but not how it is reported), invalid value types.
        - Existing API functions ``snapshot_count`` and ``snapshot_max_count`` are not covered by a requirement.
      - new issue to be created after inspection
    * - REQ_07_01
      - Is the *safety* attribute set correctly?
      - Derived requirements are checked automatically, see :need:`gd_req__req_linkage_safety`. But for the top level requirements (and also all AoU) this needs to be checked manually for correctness.
      - Proposal: YES
      - - Automated check for derived requirements; QM requirements have QM parents. AoUs are ASIL_B.
        - Discuss: :need:`comp_req__kvs__multi_instance` is QM although "operations on one instance do not affect ... any other instance" is a freedom-from-interference property.
      -
    * - REQ_07_02
      - Is the attribute *security* set correctly?
      - For component requirements this checklist item is supported by automated check: "Every requirement which satisfies a feature requirement with security attribute set to YES inherits this". But the component requirements/architecture may additionally also be subject to a :need:`wp__sw_component_security_analysis`.
      - Proposal: YES
      - - Automated check; all children of security-relevant feature requirements have ``security: YES``.
        - :need:`feat_req__persistency__confidential_storage` (security YES) has no component requirement - see REQ_10_01.
      -
    * - REQ_08_01
      - Is the requirement *verifiable*?
      - If at the time of the inspection already tests are created for the requirement, the answer is yes. This can be checked via traces, but also :need:`gd_req__req_attr_test_covered` shows this. In case the requirement is not sufficiently traced to test cases already, a test expert is invited to the inspection to give their opinion whether the requirement is formulated in a way that supports test development and the available test infrastructure is sufficient to perform the test.
      - Proposal: NO
      - - Only 17 of 65 component requirements are referenced by tests. Test expert to assess the remaining ones.
        - Not or hardly testable, to be verified according to REQ_08_02:
        - :need:`comp_req__kvs__permission_control` - absence of own access control
        - :need:`comp_req__kvs__os_abstraction` - "exclusively" through an abstraction layer
        - :need:`comp_req__kvs__pers_data_store_bnd` - "use the file API"
        - :need:`comp_req__kvs__no_dynamic_memory` - testable only with an instrumented allocator
        - Finding of version 1 on :need:`comp_req__kvs__pers_data_version` resolved (now a positive, testable statement).
      - new issue to be created after inspection
    * - REQ_08_02
      - Is the requirement verifiable by design or code review in case it is not feasibly testable?
      - In very rare cases a requirement may not be verifiable by test cases, for example a specific non-functional requirement. In this case a requirement analysis verifies the requirement by design/code review. If such a requirement is in scope of this inspection, please check this here and link to the respective review record. A test expert is invited to the inspection to confirm their opinion that the requirement is not testable.
      - Proposal: NO
      - No design/code review records exist yet for the requirements listed in REQ_08_01.
      - https://github.com/eclipse-score/persistency/issues/297
    * - REQ_09_01
      - Do the requirements that define a safety mechanism specify the error reaction leading to a safe state?
      - Alternatively to the safe state there could also be "repair" mechanisms. Also do not forget to consider REQ_05_01 for these.
      - Proposal: NO
      - - Findings of version 1 resolved for :need:`comp_req__kvs__pers_data_csum_vrfy` (error reported, data not loaded) and corruption on open (:need:`comp_req__kvs__corruption_detect_open`, :need:`comp_req__kvs__open_need_kvs_behavior`).
        - Still open: :need:`comp_req__kvs__default_val_chksum` defines the checksum, but no error reaction on a checksum mismatch of the default values file.
        - :need:`comp_req__kvs__single_process_access` defines a safety mechanism (freedom from interference) without error reaction (what happens on the second open?).
      - https://github.com/eclipse-score/persistency/issues/297
    * - REQ_10_01
      - Is the requirement description *complete* ?
      - For every requirement in the inspection, follow to its parent (feature) requirement(s) and then check if this/these are fulfilled completely by its/their linked children (component requirements, including those which are not in scope of the inspection).
      - Proposal: NO
      - - Parent feature requirements not completely fulfilled by their children:
        - :need:`feat_req__persistency__cfg` - configuration of instance identifier, storage URI, access mode, Need Defaults and storage size not covered (only snapshot max. count and Need Kvs).
        - :need:`feat_req__persistency__async_api` - asynchronous loading not covered (:need:`comp_req__kvs__async_api` covers flush only).
        - :need:`feat_req__persistency__async_completion` - completion signalling not covered (see REQ_03_01).
        - :need:`feat_req__persistency__snapshot_create` - unique snapshot IDs and data layout version in snapshots not covered.
        - :need:`feat_req__persistency__storage_backends` - only backend selection is covered, not multiple backends (only JSON is required).
        - :need:`feat_req__persistency__confidential_storage` - no component requirement; only :need:`aou_req__persistency__confidentiality` - rationale needed.
        - :need:`feat_req__persistency__update_mechanism` - multi-version jumps not explicitly covered by :need:`comp_req__kvs__pers_data_migration`.
      - new issue to be created after inspection

The following requirements in "valid" state and with "inspected" tag set are in the scope of this inspection:

.. needtable::
   :filter: docname is not None and "kvs" in docname and "requirements" in docname and status == "valid"
   :style: table
   :types: comp_req
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title


And also the following AoUs in "valid" state and with "inspected" tag set (for these please answer the questions above as if the AoUs are requirements, except question REQ_03_01):

.. needtable::
   :filter: docname is not None and "kvs" in docname and "requirements" in docname and status == "valid"
   :style: table
   :types: aou_req
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title


.. needextend:: c.this_doc() and docname is not None
   :+tags: kvs

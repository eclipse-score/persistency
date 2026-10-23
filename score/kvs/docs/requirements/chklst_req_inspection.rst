..
   # *******************************************************************************
   # Copyright (c) 2026 Contributors to the Eclipse Foundation
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
   :tags: kvs

KVS Requirement Inspection Checklist
====================================

Purpose
-------

The purpose of this requirement inspection checklist is to collect the topics to be checked during requirements inspection.

Conduct
-------

As described in the concept :need:`doc_concept__wp_inspections` the following "inspection roles" are filled:

- content responsible (author): `<https://github.com/Ahmed-Elsaka-JC>`_
- reviewer: `<https://github.com/PandaeDo>`_, `<https://github.com/vinodreddy-g>`_, `<https://github.com/arkjedrz>`_
- moderator: `<https://github.com/janis-qorix>`_
- test expert: `<https://github.com/arkjedrz>`_

The findings of the previous inspection (`#288 <https://github.com/eclipse-score/persistency/pull/288>`_)
are tracked in `#297 <https://github.com/eclipse-score/persistency/issues/297>`_.

The checklist is prefilled by the safety manager based on the state of ``afa3367e``;
the reviewers confirm or correct each item.

Checklist
---------

It is mandatory to fill in the "passed" column with "yes" or "no" for each checklist item and additionally to add in the remarks why it is passed or not passed.
In case of "no" an issue link to the issue tracking system has to be added in the last column (if not solved in the same issue).
See also :need:`doc_concept__wp_inspections` for further information about reviews in general and inspection in particular.

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
      - yes
      - All 65 requirements use the formulation "The component shall ..." of :need:`gd_temp__req_formulation`.
      -
    * - REQ_02_01
      - Is the requirement description *comprehensible* ?
      - If you think the requirement is hard to understand, comment here.
      - yes
      - The term "snapshot" (finding of the previous inspection) is now explained in the note of :need:`comp_req__kvs__snapshot_create_api`.
        For unclear wording see REQ_02_02.
      -
    * - REQ_02_02
      - Is the requirement description *unambiguous* ?
      - Especially search for "weak words" like "about", "etc.", "relevant" and others (see the internet documentation on this). This check shall be supported by tooling.
      - no
      - Wording which leaves room for interpretation:

        - :need:`comp_req__kvs__constraints`: "KVS constraints" are not defined (which parameters?).
        - :need:`comp_req__kvs__value_data_types`: range and representation of "Number" are not defined.
        - :need:`comp_req__kvs__pers_data_store_bnd`: "the file API" is not specified (OS file API, baselibs filesystem?).
        - :need:`comp_req__kvs__callback_support`: "data change events" are not defined.
        - :need:`comp_req__kvs__async_api`: "the standard API" is not defined.
        - :need:`comp_req__kvs__snapshot_source`: "live values" and the condition (when creating a snapshot) are missing.
        - :need:`comp_req__kvs__pers_data_migration`: "supported schema versions" are not listed.
      -
    * - REQ_02_03
      - Is the requirement description *atomic* ?
      - A good way to think about this is to consider if the requirement may be tested by one (positive) test case or needs more of these. The requirement formulation template should also avoid being non-atomic already. Note that there are cases where also non-atomic requirements are the better ones, for example if those are better understandable.
      - yes
      - Some requirements combine closely related statements, e.g. :need:`comp_req__kvs__key_uniqueness`, :need:`comp_req__kvs__corruption_detect_open`, :need:`comp_req__kvs__snapshot_restore_api` and :need:`comp_req__kvs__open_need_kvs_behavior` (one statement per configuration value). They stay understandable and testable as one unit.
      -
    * - REQ_02_04
      - Is the requirement description *feasible* ?
      - If at the time of the inspection the requirement has already some implementation, the answer is yes. This can be checked via traces, but also :need:`gd_req__req_attr_impl` shows this. In case the requirement has no implementation at the time of inspection (i.e. not implemented at least as "proof-of-concept"), a development expert should be invited to the Pull-Request review to explicitly check this item.
      - no
      - No implementation found in the C++ and Rust sources (state of ``afa3367e``), feasibility to be confirmed by the development expert:

        - :need:`comp_req__kvs__async_api` and :need:`comp_req__kvs__callback_support`: no asynchronous flush, no callback registration
        - :need:`comp_req__kvs__constraints` and :need:`comp_req__kvs__constraints_priority`: no configuration file
        - :need:`comp_req__kvs__pers_data_version`, :need:`comp_req__kvs__pers_data_schema` and :need:`comp_req__kvs__pers_data_migration`: no schema version in the JSON files, no migration
        - :need:`comp_req__kvs__single_process_access`: only an in-process instance registry, no inter-process lock
        - :need:`comp_req__kvs__default_storage_permissions`: the kvs tool sets no file permissions
        - :need:`comp_req__kvs__no_dynamic_memory`: the in-memory cache (``std::unordered_map<std::string, KvsValue>``, Rust ``HashMap``) allocates on ``set_value``
        - :need:`comp_req__kvs__storage_backend_select`: the Rust backend is selected at runtime via the builder, the C++ implementation has one backend
      -
    * - REQ_02_05
      - Is the requirement description *independent from implementation* ?
      - This checkpoint should improve requirements definition in the sense that the "what" is described and not the "how" - the latter should be described in architecture/design derived from the requirement. But there can also be a good reason for this, for example we would require using a file format like JSON and even specify the formatting standard already on stakeholder requirement level because we want to be compatible. A finding in this checkpoint does not mean there is a safety problem in the requirement.
      - yes
      - JSON and the file based storage (:need:`comp_req__kvs__value_serialize`, :need:`comp_req__kvs__pers_data_store_fmt`, :need:`comp_req__kvs__pers_data_schema`, :need:`comp_req__kvs__pers_data_store_bnd`) are given by the feature architecture (JSON according to RFC-8259) and are needed for compatibility of the persisted data.
        Finding REQ_04_01 of the previous inspection (JSON serialization belongs to a JSON component) is resolved by the architecture using the baselibs JSON interface.
      -
    * - REQ_03_01
      - Is the *linkage to the parent requirement* correct?
      - Linkage to correct levels and ASIL attributes is checked automatically, but it needs checking if the child requirement implements (at least) a part of the parent requirement.
      - no
      - - :need:`comp_req__kvs__pers_data_store_bnd` and :need:`comp_req__kvs__pers_data_store_fmt` do not implement a part of their parent :need:`feat_req__persistency__integrity_check`.
        - :need:`comp_req__kvs__callback_support` (callbacks on data change events) does not implement the signalling of the completion of asynchronous operations required by its parent :need:`feat_req__persistency__async_completion`.
        - The other findings of the previous inspection on parent linkage are resolved.
      -
    * - REQ_04_01
      - Is the requirement *internally and externally consistent*?
      - Does the requirement contradict other requirements within the same or higher levels? One may restrict the search to the feature for component requirements, for features to other features using same components. Is the description of the requirement consistent with all its attributes (if not already part of another check, e.g. does the title fit?).
      - no
      - - :need:`comp_req__kvs__snapshot_id_api` and :need:`comp_req__kvs__snapshot_slot_index_vali` define the slot index range fixed as ``<1..3>``, independent of the configured maximum number of snapshots (:need:`comp_req__kvs__snapshot_max_num_cfg`, ``<0..3>``, ``0`` disables snapshots).
        - :need:`comp_req__kvs__value_default` ("when no value has been set by the user") and :need:`comp_req__kvs__default_value_query` ("even if value is set by the user") describe the same API with different conditions.
        - :need:`comp_req__kvs__no_dynamic_memory` contradicts the in-memory cache required by :need:`comp_req__kvs__cached_access` and :need:`comp_req__kvs__value_set_api` unless the cache size is limited and allocated at opening.
      -
    * - REQ_05_01
      - Do the software requirements consider *timing constraints*?
      - This checkpoint encourages to think about timing constraints even if those are not explicitly mentioned in the parent requirement. If the reviewer of a requirement already knows or suspects that the code execution will be consuming a lot of time, one should think of the expectation of a "user".
      - no
      - No timing constraints are defined for open, flush, snapshot restore or the asynchronous flush, although their duration depends on the storage size (``max_storage_bytes`` exists in the implementation, but not in the requirements).
      -
    * - REQ_06_01
      - Does the requirement consider *external interfaces*?
      - The SW platform's external interfaces (to the user) are defined in the Feature Architecture, so the Feature and Component Requirements should determine the input data use and setting of output data for these interfaces. Are all output values defined?
      - no
      - The error outputs of the API are not completely defined, e.g. :need:`comp_req__kvs__value_get_api` for a key which is neither written nor has a default value, :need:`comp_req__kvs__key_remove_api` for a key which is not written, :need:`comp_req__kvs__value_set_api` for an invalid key or value, :need:`comp_req__kvs__explicit_flush` when writing fails.
        The operations of the interface (e.g. open, flush) are part of the real interfaces in the component architecture.
      - `#297 <https://github.com/eclipse-score/persistency/issues/297>`_
    * - REQ_07_01
      - Is the *safety* attribute set correctly?
      - Derived requirements are checked automatically, see :need:`gd_req__req_linkage_safety`. But for the top level requirements (and also all AoU) this needs to be checked manually for correctness.
      - yes
      - The safety attribute of the derived requirements is checked automatically. The QM requirements (:need:`comp_req__kvs__language_bindings`, :need:`comp_req__kvs__os_abstraction`, :need:`comp_req__kvs__multi_instance`, :need:`comp_req__kvs__random_access_complexity`) are derived from QM feature requirements. The component has no own AoU.
      -
    * - REQ_07_02
      - Is the attribute *security* set correctly?
      - For component requirements this checklist item is supported by automated check: "Every requirement which satisfies a feature requirement with security attribute set to YES inherits this". But the component requirements/architecture may additionally also be subject to a :need:`wp__sw_component_security_analysis`.
      - yes
      - The security attribute is set to YES for all requirements derived from feature requirements with security YES (:need:`feat_req__persistency__access_control`, :need:`feat_req__persistency__cfg`, :need:`feat_req__persistency__cfg_priority`, :need:`feat_req__persistency__reset_resistant`).
        A component security analysis is not available.
      -
    * - REQ_08_01
      - Is the requirement *verifiable*?
      - If at the time of the inspection already tests are created for the requirement, the answer is yes. This can be checked via traces, but also :need:`gd_req__req_attr_test_covered` shows this. In case the requirement is not sufficiently traced to test cases already, a test expert is invited to the inspection to give their opinion whether the requirement is formulated in a way that supports test development and the available test infrastructure is sufficient to perform the test.
      - no
      - 17 of 65 requirements are linked to tests (component integration tests in ``score/kvs/tests/test_cases``), 4 of them fully verified. The unit tests reference requirements only in comments.
        Not linked to tests are e.g. all requirements on permissions, integrity check and recovery (:need:`comp_req__kvs__pers_data_csum`, :need:`comp_req__kvs__pers_data_csum_vrfy`, :need:`comp_req__kvs__corruption_detect_open`, :need:`comp_req__kvs__open_need_kvs_behavior`, :need:`comp_req__kvs__reset_resistant_write`, :need:`comp_req__kvs__atomic_store`) and most snapshot requirements.
        :need:`comp_req__kvs__default_val_chksum` is linked as fully verified, but the test only checks that the checksum file is created.
        To be assessed by the test expert.
      -
    * - REQ_08_02
      - Is the requirement verifiable by design or code review in case it is not feasibly testable?
      - In very rare cases a requirement may not be verifiable by test cases, for example a specific non-functional requirement. In this case a requirement analysis verifies the requirement by design/code review. If such a requirement is in scope of this inspection, please check this here and link to the respective review record. A test expert is invited to the inspection to confirm their opinion that the requirement is not testable.
      - no
      - Not or not feasibly testable, no review record available:

        - :need:`comp_req__kvs__permission_control`: not implementing own access controls (finding of the previous inspection)
        - :need:`comp_req__kvs__os_abstraction`: access exclusively through an OS abstraction layer
        - :need:`comp_req__kvs__no_dynamic_memory`: absence of heap allocation after opening
        - :need:`comp_req__kvs__random_access_complexity`: time complexity
      - `#297 <https://github.com/eclipse-score/persistency/issues/297>`_
    * - REQ_09_01
      - Do the requirements that define a safety mechanism specify the error reaction leading to a safe state?
      - Alternatively to the safe state there could also be "repair" mechanisms. Also do not forget to consider REQ_05_01 for these.
      - no
      - - :need:`comp_req__kvs__pers_data_csum_vrfy`, :need:`comp_req__kvs__corruption_detect_open` and :need:`comp_req__kvs__open_need_kvs_behavior` define the error reaction.
        - :need:`comp_req__kvs__default_val_chksum`: no error reaction defined (finding of the previous inspection).
        - :need:`comp_req__kvs__single_process_access`: the reaction when a second process opens the instance is not defined.
      - `#297 <https://github.com/eclipse-score/persistency/issues/297>`_
    * - REQ_10_01
      - Is the requirement description *complete* ?
      - For every requirement in the inspection, follow to its parent (feature) requirement(s) and then check if this/these are fulfilled completely by its/their linked children (component requirements, including those which are not in scope of the inspection).
      - no
      - Parent requirements not completely covered by component requirements:

        - :need:`feat_req__persistency__confidential_storage`: no component requirement
        - :need:`feat_req__persistency__cfg`: no component requirements for the configuration items instance identifier, storage URI, access mode, need defaults and storage size
        - :need:`feat_req__persistency__snapshot_create`: "Snapshots shall also include the version of the data layout" is not covered
        - :need:`feat_req__persistency__async_api`: only the flush is asynchronous, loading is not covered
      -

Note: If a Review ID is not applicable for your requirement, then state ""n/a" in status and comment accordingly in remarks.

The following requirements in "valid_inspected" state are in the scope of this inspection:

.. needtable::
   :filter: docname is not None and "kvs" in docname and "requirements" in docname
   :style: table
   :types: comp_req
   :tags: kvs
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title

And also the following AoUs in "valid_inspected" state (for these please answer the questions above as if the AoUs are requirements, except question REQ_03_01):

.. needtable::
   :filter: docname is not None and "kvs" in docname and "requirements" in docname
   :style: table
   :types: aou_req
   :tags: kvs
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title

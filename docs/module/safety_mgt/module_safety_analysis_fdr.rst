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


Persistency Safety Analysis Checklist
=====================================

.. document:: Persistency Safety Analysis Checklist
   :id: doc__persistency_safety_analysis_fdr
   :status: draft
   :version: 2
   :safety: ASIL_B
   :security: YES
   :realizes: wp__fdr_reports[version==1]
   :tags: persistency


**Purpose**

The purpose of this Safety Analysis (DFA and FMEA) formal review report is to collect the topics to be checked during verification of the Safety Analysis.

**Conduct**

As described in :need:`wf__p_formal_rv`, the formal document review is performed by an "external" safety manager:

- reviewer: Alexander Schemmel (`<https://github.com/aschemmel-tech>`_)
- scope: the safety analyses of the feature Persistency (:need:`doc__persistency_fmea`, :need:`doc__persistency_dfa`) and of the component KVS (:need:`doc__kvs_fmea`, :need:`doc__kvs_dfa`) and their results

**Checklist**

Please note that it is mandatory to fill in the "passed" column with "yes" or "no" for each checklist item and additional to add in the remarks why it is passed or not passed. In case of "no" an issue link to the issue tracking system has to be added in the last column. See also :need:`doc_concept__wp_inspections` for further information about reviews in general and inspection in particular.


.. list-table:: General Checklist
      :header-rows: 1
      :widths: 10,30,10,30,20

      * - ID
        - Safety analysis activity
        - Compliant to ISO 26262?
        - Reference
        - Comment

      * - Gen 1
        - Are the safety analysis performed according to the defined process and templates? See :need:`gd_req__saf_structure` and also :need:`gd_temp__feat_saf_fmea` and :need:`gd_temp__feat_saf_dfa`
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_841>`, :need:`[[title]] <std_req__iso26262__analysis_849>`, :need:`[[title]] <std_req__iso26262__analysis_8410>`, :need:`[[title]] <std_req__iso26262__analysis_748>`
        - The feature FMEA and DFA use the templates :need:`gd_temp__feat_saf_fmea` and :need:`gd_temp__feat_saf_dfa`. The
          component FMEA and DFA refer to the feature analyses with the rationale that KVS has no internal components (see
          tailoring notes in :need:`doc__persistency_safety_plan`).

      * - Gen 2
        - Are the safety analysis performed in a systematic way to identify the potential dependent failures / failure modes and their effects? Are the failure effect and the mitigation described?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_849>`, :need:`[[title]] <std_req__iso26262__analysis_8410>`
        - The applicability of all fault models and failure initiators is documented in tables. Several "not applicable"
          rationales are generic ("Failure initiator not applicable at persistency, so no mitigation is needed.") or argue with
          "developed fully deterministic" (e.g. FMEA EX_01_01, DFA CO_01_02), which does not exclude systematic faults. Rework
          in `#481 <https://github.com/eclipse-score/persistency/pull/481>`_.

      * - Gen 3
        - Is the result of the safety analysis indicate if the safety requirements are complied?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_842>`
        - Each FMEA and DFA entry states ``sufficient: yes``, but the analyses contain no overall result statement whether the
          safety requirements of the feature are complied with. Rework in `#481
          <https://github.com/eclipse-score/persistency/pull/481>`_.

      * - Gen 4
        - Are the mitigations effective and implemented?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_844>`
        - All identified failures are mitigated by :need:`aou_req__persistency__error_handling`. The API reports detected errors
          to the application (C++ ``score::Result``, Rust ``Result``), so the application can handle the unavailability as
          required by the AoU.

      * - Gen 5
        - Are all AoU's that are used as mitigation's created and covered in the safety manual?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_845>`
        - :need:`aou_req__persistency__error_handling` is part of :need:`doc__persistency_safety_manual`. It is tagged as
          assumption on the environment instead of on the user, correction in `#541
          <https://github.com/eclipse-score/persistency/pull/541>`_.

      * - Gen 6
        - Are additional safety-related test cases determined by potential results of the safety analyses?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_847>`
        - No test cases are derived from the analyses. The safety mechanism integrity check
          (:need:`feat_req__persistency__integrity_check`, checksum of the persisted data) is not referenced as mitigation in
          the FMEA. Rework in `#481 <https://github.com/eclipse-score/persistency/pull/481>`_.


.. list-table:: DFA Checklist
      :header-rows: 1
      :widths: 10,30,10,30,20

      * - ID
        - Safety analysis activity
        - Compliant to ISO 26262?
        - Reference
        - Comment

      * - DFA 1
        - Are the potential dependent failures identified by performing a DFA?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_741>`
        - The DFA is performed on feature level (:need:`doc__persistency_dfa`) with the failure initiators of
          :need:`gd_guidl__dfa_failure_initiators`, the component DFA (:need:`doc__kvs_dfa`) refers to it.

      * - DFA 2
        - Is it plausible that each potential identified dependent failure that has been identified, will lead to a dependent failure which cause a violation of FFI?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_742>`
        - The one identified dependent failure (:need:`feat_saf_dfa__persistency__execution_blocking`) is plausible: persistency
          runs in the execution context of the calling application.

      * - DFA 3
        - Are applicable operational situations and operating modes considered?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_743>`
        - Operating modes (startup with loading of the data and defaults, runtime access, flush, snapshot restore) are not
          considered explicitly in the DFA. Rework in `#481 <https://github.com/eclipse-score/persistency/pull/481>`_.

      * - DFA 4
        - Are the failure initiators :need:`[[title]] <gd_guidl__dfa_failure_initiators>` suitable and applied?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_744>`
        - The failure initiators are applied in tables. The shared resources (file system and storage, heap memory, baselibs)
          are declared not applicable on feature level and deferred to the platform DFA, which is not referenced. Several
          rationales are generic, see Gen 2.

      * - DFA 5
        - Is a rationale provided for each identified potential dependent failure?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_745>`
        - A rationale is given for the identified dependent failure :need:`feat_saf_dfa__persistency__execution_blocking`.

      * - DFA 6
        - Are measures defined to resolve the identified potential dependent failures?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_746>`, :need:`[[title]] <std_req__iso26262__analysis_747>`, :need:`[[title]] <std_req__iso26262__analysis_843>`
        - The identified dependent failure is mitigated by :need:`aou_req__persistency__error_handling`.

      * - DFA 7
        - Can be the required level of independence shown for the identified potential dependent failures?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_748>`
        - Independence from other software elements using the same shared resources (e.g. other processes writing the persisted
          files) is not shown, as the shared resources are deferred to the platform DFA (see DFA 4).


.. list-table:: FMEA Checklist
      :header-rows: 1
      :widths: 10,30,10,30,20

      * - ID
        - Safety analysis activity
        - Compliant to ISO 26262?
        - Reference
        - Comment

      * - FMEA 1
        - Are the fault models suitable and applied for the FMEA? See :need:`gd_guidl__fault_models` and also :need:`gd_req__saf_structure`
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_846>`
        - All fault models of :need:`gd_guidl__fault_models` (MF_01_01 to EX_01_06) are listed with applicability; the
          applicable ones are analysed against the dynamic feature architecture. For the quality of the rationales see Gen 2.

      * - FMEA 2
        - Are measures defined to resolve the identified faults?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_843>`
        - All identified failure modes are mitigated by :need:`aou_req__persistency__error_handling`. For the missing reference
          to the integrity check see Gen 6.

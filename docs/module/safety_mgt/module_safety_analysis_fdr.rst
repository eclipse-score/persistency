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
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_841>`, :need:`[[title]] <std_req__iso26262__analysis_849>`, :need:`[[title]] <std_req__iso26262__analysis_8410>`, :need:`[[title]] <std_req__iso26262__analysis_748>`
        - The analyses use the templates and are located on feature level (``docs/features/persistency/safety_analysis``)
          and on component level (``score/kvs/docs/safety_analysis``) according to :need:`gd_req__saf_structure`. Deviation:

          - :need:`doc__kvs_fmea` and :need:`doc__kvs_dfa` and their entries rated ``sufficient: no`` have the status
            ``valid``. According to :need:`gd_req__saf_attr_mitigation_issue` and :need:`doc_getstrt__safety_analysis`
            the status is ``draft`` until all mitigations are sufficient. Tracked by the mitigation issues of DFA 6
            and FMEA 2 (`#760 <https://github.com/eclipse-score/persistency/issues/760>`__, `#761 <https://github.com/eclipse-score/persistency/issues/761>`__, `#762 <https://github.com/eclipse-score/persistency/issues/762>`__, `#763 <https://github.com/eclipse-score/persistency/issues/763>`__, `#764 <https://github.com/eclipse-score/persistency/issues/764>`__, `#765 <https://github.com/eclipse-score/persistency/issues/765>`__).

      * - Gen 2
        - Are the safety analysis performed in a systematic way to identify the potential dependent failures / failure modes and their effects? Are the failure effect and the mitigation described?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_849>`, :need:`[[title]] <std_req__iso26262__analysis_8410>`
        - All fault models and failure initiators are listed with applicability and a specific rationale on feature and
          component level. Each entry describes failure effect, root cause and mitigation in the content.

      * - Gen 3
        - Is the result of the safety analysis indicate if the safety requirements are complied?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_842>`
        - Each entry states with ``mitigated_by`` and ``sufficient`` whether the violated architecture element complies
          with the safety requirements. An overall result is not part of the templates (:need:`gd_temp__feat_saf_fmea`,
          :need:`gd_temp__feat_saf_dfa`); according to :need:`wp__verification_module_ver_report` it is part of the
          verification report (DFA and FMEA report with the identified mitigations).

      * - Gen 4
        - Are the mitigations effective and implemented?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_844>`
        - The mitigations are specified as safety requirements and AoUs, linked via ``mitigated_by``, and their
          effectiveness is argued in the content of each entry rated ``sufficient: yes``. The implementation of the
          linked requirements is verified by the requirement-based tests and reported in
          :need:`wp__verification_module_ver_report`.

      * - Gen 5
        - Are all AoU's that are used as mitigation's created and covered in the safety manual?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_845>`
        - The AoUs used as mitigations (:need:`aou_req__persistency__error_handling`, :need:`aou_req__persistency__flush`,
          :need:`aou_req__persistency__instance_separation`, :need:`aou_req__persistency__start_without_data`,
          :need:`aou_req__kvs__filesystem`) are part of the AoU tables of :need:`doc__persistency_safety_manual`.

      * - Gen 6
        - Are additional safety-related test cases determined by potential results of the safety analyses?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_847>`
        - All mitigations are safety requirements (e.g. :need:`feat_req__persistency__integrity_check`) or AoUs. The
          safety-related test cases are derived from these requirements; the AoUs are verified by the user. The analyses
          do not identify failures which need test cases beyond the requirement-based tests.


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
        - The DFA is performed on feature level (:need:`doc__persistency_dfa`) against
          :need:`feat_arc_sta__persistency__static` and on component level (:need:`doc__kvs_dfa`) against
          :need:`comp_arc_sta__persistency_kvs__sv` with the failure initiators of :need:`gd_guidl__dfa_failure_initiators`.

      * - DFA 2
        - Is it plausible that each potential identified dependent failure that has been identified, will lead to a dependent failure which cause a violation of FFI?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_742>`
        - The identified dependent failures (six on feature level, seven on component level) are plausible, e.g. blocking of
          the calling context (including the wait for the instance lock), shared instances within a process, the
          process-wide instance registry (:need:`comp_saf_dfa__kvs__instance_registry`), the instance state shared by
          handles (:need:`comp_saf_dfa__kvs__shared_instance_state`), access from multiple processes, memory depletion.

      * - DFA 3
        - Are applicable operational situations and operating modes considered?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_743>`
        - The operating modes open and reload, runtime access, flush (including the asynchronous flush) and snapshot
          restore are considered, e.g. :need:`comp_saf_fmea__kvs__corrupt_on_open`,
          :need:`comp_saf_dfa__kvs__async_flush`.

      * - DFA 4
        - Are the failure initiators :need:`[[title]] <gd_guidl__dfa_failure_initiators>` suitable and applied?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_744>`
        - The failure initiators are applied with rationales on feature and component level. The shared resources
          (file system and storage, used libraries, operating system) are considered in the platform DFA according to
          :need:`doc_concept__safety_analysis`; they are verified by the platform safety analysis FDR.

      * - DFA 5
        - Is a rationale provided for each identified potential dependent failure?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_745>`
        - Each identified dependent failure has a rationale in its content.

      * - DFA 6
        - Are measures defined to resolve the identified potential dependent failures?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_746>`, :need:`[[title]] <std_req__iso26262__analysis_747>`, :need:`[[title]] <std_req__iso26262__analysis_843>`
        - Measures are linked for all identified dependent failures. :need:`comp_saf_dfa__kvs__callback_deadlock` is rated
          ``sufficient: no`` without ``mitigation_issue`` and with ``status: valid`` instead of ``draft``
          (:need:`gd_req__saf_attr_mitigation_issue`). Issue: `#760 <https://github.com/eclipse-score/persistency/issues/760>`__.

      * - DFA 7
        - Can be the required level of independence shown for the identified potential dependent failures?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_748>`
        - The identified dependent failures are cascading failures between the users of the persistency (threads and
          processes using the same instance or the same files). Freedom from interference is shown by the linked
          measures, e.g. :need:`feat_req__persistency__multiple_app`, :need:`feat_req__persistency__access_control`,
          :need:`aou_req__persistency__instance_separation`. Open measure: see DFA 6.


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
        - All fault models of :need:`gd_guidl__fault_models` (MF_01_01 to EX_01_06) are listed with applicability on
          feature level against the dynamic views and on component level against the component mechanisms.

      * - FMEA 2
        - Are measures defined to resolve the identified faults?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_843>`
        - Measures are linked for all identified failure modes. :need:`comp_saf_fmea__kvs__flush_skipped`,
          :need:`comp_saf_fmea__kvs__constraint_exceeded`, :need:`comp_saf_fmea__kvs__constraint_cfg_wrong`,
          :need:`comp_saf_fmea__kvs__migration_wrong` and :need:`comp_saf_fmea__kvs__tool_wrong_output` are rated
          ``sufficient: no`` without ``mitigation_issue`` and with ``status: valid`` instead of ``draft``
          (:need:`gd_req__saf_attr_mitigation_issue`). Issues: `#761 <https://github.com/eclipse-score/persistency/issues/761>`__, `#762 <https://github.com/eclipse-score/persistency/issues/762>`__, `#763 <https://github.com/eclipse-score/persistency/issues/763>`__, `#764 <https://github.com/eclipse-score/persistency/issues/764>`__,
          `#765 <https://github.com/eclipse-score/persistency/issues/765>`__.

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
          and on component level (``score/kvs/docs/safety_analysis``) according to :need:`gd_req__saf_structure`. Deviations:

          - All four analyses have the status ``valid``, although the verification is not finished and entries are rated
            ``sufficient: no``. According to :need:`gd_guidl__documentation` (lifecycle model 2) and
            :need:`doc_getstrt__safety_analysis` the status is ``draft`` until the verification is done and no issues are
            open.
          - The tailoring in :need:`doc__persistency_safety_plan` states that the component analyses refer to the feature
            analyses. :need:`doc__kvs_fmea` and :need:`doc__kvs_dfa` contain an own analysis of the component mechanisms
            and refer back to this tailoring for their scope.

      * - Gen 2
        - Are the safety analysis performed in a systematic way to identify the potential dependent failures / failure modes and their effects? Are the failure effect and the mitigation described?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_849>`, :need:`[[title]] <std_req__iso26262__analysis_8410>`
        - All fault models and failure initiators are listed with applicability and a specific rationale on feature and
          component level. Each entry describes failure effect, root cause and mitigation in the content.

      * - Gen 3
        - Is the result of the safety analysis indicate if the safety requirements are complied?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_842>`
        - Each entry is rated with ``sufficient``, but none of the four analyses contains an overall result statement
          whether the safety requirements of the feature and the component are complied with.

      * - Gen 4
        - Are the mitigations effective and implemented?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_844>`
        - Several entries are rated ``sufficient: yes`` with ``mitigated_by``, while their content states that the
          mitigation is not implemented, e.g. :need:`feat_saf_dfa__persistency__multi_process`,
          :need:`feat_saf_fmea__persistency__constraint_max`, :need:`comp_saf_fmea__kvs__flush_interrupted`,
          :need:`comp_saf_fmea__kvs__corrupt_on_open`, :need:`comp_saf_fmea__kvs__default_corrupt`,
          :need:`comp_saf_dfa__kvs__file_access`, :need:`comp_saf_dfa__kvs__async_flush`,
          :need:`comp_saf_dfa__kvs__multi_process`, :need:`comp_saf_dfa__kvs__memory_depletion`. According to
          :need:`gd_req__saf_attr_mitigated_by` a mitigation which is not implemented is not linked via ``mitigated_by``.
          The note in :need:`feat_saf_dfa__persistency__callback_deadlock` (runtime allocation) does not match the
          analysed deadlock.

      * - Gen 5
        - Are all AoU's that are used as mitigation's created and covered in the safety manual?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_845>`
        - The feature AoUs (:need:`aou_req__persistency__error_handling`, :need:`aou_req__persistency__flush`,
          :need:`aou_req__persistency__instance_separation`, :need:`aou_req__persistency__start_without_data`) are part of
          :need:`doc__persistency_safety_manual`. :need:`aou_req__kvs__filesystem` (mitigation of
          :need:`comp_saf_fmea__kvs__unflushed_lost`) is tagged ``Persistency KVS`` and therefore not selected by the AoU
          tables of the safety manual, which filter on the tag ``persistency``.

      * - Gen 6
        - Are additional safety-related test cases determined by potential results of the safety analyses?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_847>`
        - The safety mechanisms are linked as mitigations (e.g. :need:`feat_req__persistency__integrity_check`), but no
          additional test cases are determined from the analyses.


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
        - The identified dependent failures (six on feature level, five on component level) are plausible, e.g. blocking of
          the calling context, shared instances within a process, access from multiple processes, memory depletion.

      * - DFA 3
        - Are applicable operational situations and operating modes considered?
        - YES
        - :need:`[[title]] <std_req__iso26262__analysis_743>`
        - The operating modes open and reload, runtime access, flush (including the asynchronous flush) and snapshot
          restore are considered, e.g. :need:`comp_saf_fmea__kvs__corrupt_on_open`,
          :need:`comp_saf_dfa__kvs__async_flush`.

      * - DFA 4
        - Are the failure initiators :need:`[[title]] <gd_guidl__dfa_failure_initiators>` suitable and applied?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_744>`
        - The failure initiators are applied with rationales. The shared resources (file system and storage, heap memory,
          used libraries, operating system) are deferred to the platform DFA without a reference to
          :need:`doc__score_platform_dfa`, which has the status ``draft``.

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
          ``sufficient: no`` without ``mitigation_issue`` (:need:`gd_req__saf_attr_mitigation_issue`).

      * - DFA 7
        - Can be the required level of independence shown for the identified potential dependent failures?
        - NO
        - :need:`[[title]] <std_req__iso26262__analysis_748>`
        - Independence from other processes is specified (:need:`feat_req__persistency__multiple_app`,
          :need:`feat_req__persistency__access_control`), but not implemented (see Gen 4). The independence regarding the
          shared resources depends on the platform DFA (see DFA 4).


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
          ``sufficient: no`` without ``mitigation_issue`` (:need:`gd_req__saf_attr_mitigation_issue`).

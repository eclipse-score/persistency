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

Verification Report
===================

.. document:: Persistency Verification Report
   :id: doc__persistency_verification_report
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__verification_module_ver_report[version==1]
   :tags: persistency

.. mod:: Persistency Module
       :id: mod__persistency
       :status: valid
       :safety: ASIL_B
       :security: NO
       :includes: comp__persistency_kvs[version==1]

.. mod_insp:: KVS Requirements Inspection Record
       :id: mod_insp__kvs_req
       :safety: ASIL_B
       :security: NO
       :status: valid
       :inspection_type: requirements
       :inspection_state: approved
       :checklist_ref: gd_chklst__req_inspection
       :reviewers: kvs-reviewer-a,kvs-reviewer-b
       :checklist_type: req
       :findings_total: 0
       :findings_open: 0
       :inspection_date: 2026-07-07
       :belongs_to: mod__persistency
       :inspects: comp_req__kvs__key_naming[version==1], comp_req__kvs__value_default[version==1], aou_req__persistency__error_handling[version==1]
       :evidence: doc__kvs_req_inspection

.. mod_insp:: Persistency Architecture Inspection Record
       :id: mod_insp__persistency_arch
       :safety: ASIL_B
       :security: NO
       :status: valid
       :inspection_type: architecture
       :inspection_state: approved
       :checklist_ref: gd_chklst__arch_inspection_checklist
       :reviewers: feature-reviewer-a,feature-reviewer-b
       :checklist_type: arc
       :findings_total: 0
       :findings_open: 0
       :inspection_date: 2026-07-07
       :belongs_to: mod__persistency
       :inspects: feat_arc_sta__persistency__static[version==1], comp_arc_sta__kvs__static_view
       :evidence: doc__persistency_arc_inspection, doc__kvs_arc_inspection

.. mod_insp:: KVS Implementation Inspection Record
       :id: mod_insp__kvs_impl
       :safety: ASIL_B
       :security: NO
       :status: valid
       :inspection_type: implementation
       :inspection_state: approved
       :checklist_ref: gd_chklst__impl_inspection_checklist
       :reviewers: kvs-impl-reviewer-a
       :checklist_type: impl
       :findings_total: 0
       :findings_open: 0
       :inspection_date: 2026-07-07
       :belongs_to: mod__persistency
       :inspects: doc__kvs_detailed_design, comp_arc_dyn__kvs__dynamic_view
       :evidence: doc__kvs_impl_inspection

.. mod_ver_report:: Persistency Module Verification Summary
       :id: mod_vrep__persistency_report
       :safety: ASIL_B
       :security: NO
       :status: valid
       :verification_method: test_and_inspection
       :requirements_coverage_percent: 100
       :structural_coverage_percent: 0
       :branch_coverage_percent: 0
       :verdict: open
       :report_version: 1
       :release_baseline: main
       :belongs_to: mod__persistency
       :contains: mod_insp__kvs_req, mod_insp__persistency_arch, mod_insp__kvs_impl
       :covers: comp_req__kvs__key_naming[version==1], feat_arc_sta__persistency__static[version==1], doc__kvs_detailed_design, feat_saf_dfa__persistency__execution_blocking[version==1], feat_saf_fmea__persistency__err_handl[version==1]
       :evidence: doc__persistency_verification_report, doc__persistency_safety_analysis_fdr
       :realizes: wp__verification_module_ver_report[version==1]

This verification report is based on the verification plan.
It covers the persistency module and all linked component/feature verification evidence.

Verification Report contains:

**1. Verification Coverage**

**1.1. on Requirements**

.. needtable:: Requirements and AoU Traceability Coverage
       :filter: type in ["comp_req", "aou_req"] and docname is not None and "kvs/docs/requirements" in docname and status == "valid"
       :style: table
       :sort: id
       :columns: id as "ID";status as "Status";tags as "Tags";fully_verifies_back as "Fully Verified By Tests";partially_verifies_back as "Partially Verified By Tests";inspects_back as "Inspected By"

**1.2. on Architecture**

.. needtable:: Architecture Verification Coverage
       :filter: type in ["feat_arc_sta", "feat_arc_dyn", "comp_arc_sta", "comp_arc_dyn"] and status == "valid" and ("persistency" in id or "kvs" in id)
       :style: table
       :sort: id
       :columns: id as "ID";type_name as "Type";status as "Status";fully_verifies_back as "Fully Verified By Tests";partially_verifies_back as "Partially Verified By Tests";inspects_back as "Inspected By"

**1.3. on Detailed Design**

.. needtable:: Detailed Design Verification Coverage
       :filter: id == "doc__kvs_detailed_design"
       :style: table
       :columns: id as "ID";status as "Status";inspects_back as "Inspected By";realizes as "Realizes"

The above requirement, architecture, and detailed design tables are generated from needs relations and inspection records.

**2. DFA Report**

.. needtable:: Performed Feature DFA Analyses
       :filter: "persistency" in id and type == "feat_saf_dfa" and is_external == False
       :style: table
       :sort: id
       :columns: id as "ID";mitigated_by as "Mitigation";sufficient as "Sufficient";status as "Status"
       :colwidths: 40,30,15,15

**3. Safety Analysis Report**

.. needtable:: Performed Feature FMEA Analyses
       :filter: "persistency" in id and type == "feat_saf_fmea" and is_external == False
       :style: table
       :sort: id
       :columns: id as "ID";mitigated_by as "Mitigation";sufficient as "Sufficient";status as "Status"
       :colwidths: 40,30,15,15

**4. Unit Verification Coverage**

**4.1. Structural Coverage**

Automatic structural/branch coverage needs are currently not available in this repository's needs dataset.
This section remains manual until unit coverage artifacts are exported as needs.

**4.2. Static Code Analysis**

Automatic static-analysis findings needs are currently not available in this repository's needs dataset.
This section remains manual until static-analysis artifacts are exported as needs.

**4.3. Manual Code Inspection**

.. needtable:: Inspection Records (Requirements, Architecture, Implementation)
       :filter: type == "mod_insp" and ("persistency" in id or "kvs" in id)
       :style: table
       :sort: id
       :columns: id as "Inspection ID";inspection_type as "Type";inspection_state as "State";findings_open as "Open Findings";inspects as "Inspected Artifacts";evidence as "Evidence"

**5. Software component qualification verification report**

No separate pre-developed software qualification report is currently linked in this module.

**6. Test results**

.. needpie:: Test Results
       :labels: passed, failed, skipped
       :colors: green, red, orange

       type == 'testcase' and result == 'passed'
       type == 'testcase' and result == 'failed'
       type == 'testcase' and result == 'skipped'

.. needtable:: Test Result Per Testcase
       :filter: type == "testcase"
       :style: table
       :sort: id
       :columns: id as "Need ID";name as "Testcase";result as "Result";fully_verifies as "Fully Verifies";partially_verifies as "Partially Verifies"

**7. Test logs**

.. needtable:: Test Logs Per Testcase
       :filter: type == "testcase"
       :style: table
       :sort: id
       :columns: id as "Need ID";name as "Testcase";result_text as "Result Text";file as "File";line as "Line"

**Note1:** The verification report is valid for the module version tagged together with the report

**Note2:** All the above lists are generated automatically

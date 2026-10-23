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

Persistency Security Plan
*************************

.. document:: Persistency Security Plan
   :id: doc__persistency_security_plan
   :status: valid
   :version: 2
   :safety: ASIL_B
   :security: YES
   :realizes: wp__module_security_plan[version==1]
   :tags: persistency

Security Management Context
===========================

This Security Plan adds to the :need:`gd_guidl__security_plan_definitions` all the module development relevant work products needed for ISO SAE 21434 conformity.

Security Management Scope
=========================

This Security Plan's scope is a SW module of the SW platform :ref:`persistency_module_documentation`.
The module consists of one or more SW components and will be qualified as an OoC (Out of Context).

Security Management Roles
=========================

.. list-table:: Module roles
        :header-rows: 1

        * - Role
          - Assignee

        * - Security Manager
          - Karthik Vanka

        * - Module Project Manager (= Feature team lead)
          - Uwe Maucher

Tailoring
=========

Additional to the tailoring in the SW platform project as defined in the :need:`gd_guidl__security_plan_definitions` we define here the additional tailoring on module level.

- Excluded for this module are additionally the following work products (and their related requirements):

  - none

- Notes on the execution of the planned work products:

  - :need:`wp__audit_report_security` is tailored out on platform level (see the work product definition), therefore no
    module security audit is planned.
  - The component security analysis refers to the feature security analysis (:need:`doc__persistency_stride`),
    because the KVS component has no internal components (:need:`doc__kvs_component_architecture`).
  - The detailed design is captured in the source code (public API headers and traits, source code comments) according to
    :need:`gd_guidl__implementation`. :need:`doc__kvs_detailed_design` documents the decomposition into units.

Security Module Work Products
=============================

One set of work products for the module and one set for each component of the module:

.. list-table:: Module Work Products
        :header-rows: 1

        * - Work Product Id
          - Link to process
          - Process status
          - Link to issue
          - Link to WP
          - WP status

        * - :need:`wp__module_security_plan`
          - :need:`gd_guidl__security_plan_definitions`
          - :ndf:`copy('status', need_id='gd_guidl__security_plan_definitions')`
          - n/a
          - this document
          - :ndf:`copy('status', need_id='doc__persistency_security_plan')`

        * - :need:`wp__module_security_package`
          - :need:`gd_guidl__security_package`
          - :ndf:`copy('status', need_id='gd_guidl__security_package')`
          - n/a
          - this document (including the linked documentation)
          - :ndf:`copy('status', need_id='doc__persistency_security_plan')`

        * - :need:`wp__fdr_reports_security` (module Security Plan)
          - :need:`gd_chklst__security_plan`
          - :ndf:`copy('status', need_id='gd_chklst__security_plan')`
          - n/a
          - :need:`doc__persistency_security_plan_fdr`
          - :ndf:`copy('status', need_id='doc__persistency_security_plan_fdr')`

        * - :need:`wp__fdr_reports_security` (module Security Package)
          - :need:`gd_chklst__security_package`
          - :ndf:`copy('status', need_id='gd_chklst__security_package')`
          - n/a
          - :need:`doc__persistency_sec_pkg_fdr`
          - :ndf:`copy('status', need_id='doc__persistency_sec_pkg_fdr')`

        * - :need:`wp__fdr_reports_security` (module's Security Analyses)
          - :need:`gd_chklst__security_analysis`
          - :ndf:`copy('status', need_id='gd_chklst__security_analysis')`
          - n/a
          - Not yet available, the review requires the completed feature security analysis :need:`doc__persistency_stride`
          - n/a

        * - :need:`wp__audit_report_security`
          - performed by external experts
          - n/a
          - n/a
          - Tailored out on platform level, see Tailoring
          - n/a

        * - :need:`wp__module_security_manual`
          - :need:`gd_temp__module_security_manual`
          - :ndf:`copy('status', need_id='gd_temp__module_security_manual')`
          - `#321 <https://github.com/eclipse-score/persistency/issues/321>`__
          - :need:`doc__persistency_security_manual`
          - :ndf:`copy('status', need_id='doc__persistency_security_manual')`

        * - :need:`wp__verification_module_ver_report`
          - :need:`gd_temp__mod_ver_report`
          - :ndf:`copy('status', need_id='gd_temp__mod_ver_report')`
          - <Link to issue>
          - :need:`mod_vrep__persistency__report`
          - :ndf:`copy('status', need_id='mod_vrep__persistency__report')`

        * - :need:`wp__module_sw_release_note`
          - :need:`gd_temp__rel_mod_rel_note`
          - :ndf:`copy('status', need_id='gd_temp__rel_mod_rel_note')`
          - n/a
          - :need:`doc__persistency_release_note`
          - :ndf:`copy('status', need_id='doc__persistency_release_note')`

        * - Module user manual (``wp__module_user_manual``, defined after process_description v2.1.2; until then
            realized as :need:`wp__training_path`)
          - Release management guidance
          - n/a
          - n/a
          - :need:`doc__persistency_user_manual`
          - :ndf:`copy('status', need_id='doc__persistency_user_manual')`

        * - :need:`wp__sw_module_sbom`
          - :need:`wf__cr_mt_security_sbom`
          - :ndf:`copy('status', need_id='wf__cr_mt_security_sbom')`
          - n/a
          - Bazel target ``//:sbom_product`` (SPDX 2.3 and CycloneDX 1.6 SBOM of ``//score/kvs:kvs`` and
            ``//score/kvs/rust_kvs:rust_kvs``), see :ref:`persistency_sbom`
          - Generated on demand with ``bazel build //:sbom_product``, not yet part of the release workflow


.. list-table:: Component KVS Work Products
        :header-rows: 1

        * - Work Product Id
          - Link to process
          - Process status
          - Link to issue
          - Link to WP
          - WP status

        * - :need:`wp__requirements_comp`
          - :need:`gd_temp__req_comp_req`
          - :ndf:`copy('status', need_id='gd_temp__req_comp_req')`
          - n/a
          - :need:`doc__kvs_requirements`
          - :ndf:`copy('status', need_id='doc__kvs_requirements')`

        * - :need:`wp__requirements_comp_aou`
          - :need:`gd_temp__req_aou_req`
          - :ndf:`copy('status', need_id='gd_temp__req_aou_req')`
          - n/a
          - :need:`doc__kvs_requirements`
          - :ndf:`copy('status', need_id='doc__kvs_requirements')`

        * - :need:`wp__requirements_inspect`
          - :need:`gd_chklst__req_inspection`
          - :ndf:`copy('status', need_id='gd_chklst__req_inspection')`
          - n/a
          - :need:`doc__kvs_req_inspection`
          - :ndf:`copy('status', need_id='doc__kvs_req_inspection')`

        * - :need:`wp__component_arch`
          - :need:`gd_temp__arch_comp`
          - :ndf:`copy('status', need_id='gd_temp__arch_comp')`
          - n/a
          - :need:`doc__kvs_component_architecture`
          - :ndf:`copy('status', need_id='doc__kvs_component_architecture')`

        * - :need:`wp__sw_component_security_analysis`
          - :need:`gd_guidl__security_analysis`
          - :ndf:`copy('status', need_id='gd_guidl__security_analysis')`
          - n/a
          - Covered by the feature security analysis :need:`doc__persistency_stride`, see Tailoring
          - :ndf:`copy('status', need_id='doc__persistency_stride')`

        * - :need:`wp__sw_arch_verification`
          - :need:`gd_chklst__arch_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__arch_inspection_checklist')`
          - `#321 <https://github.com/eclipse-score/persistency/issues/321>`__
          - :need:`doc__kvs_arc_inspection`
          - :ndf:`copy('status', need_id='doc__kvs_arc_inspection')`

        * - :need:`wp__sw_implementation`
          - :need:`gd_guidl__implementation`
          - :ndf:`copy('status', need_id='gd_guidl__implementation')`
          - `#321 <https://github.com/eclipse-score/persistency/issues/321>`__
          - :need:`doc__kvs_detailed_design` & `source code <https://github.com/eclipse-score/persistency/tree/main/score/kvs>`__
          - :ndf:`copy('status', need_id='doc__kvs_detailed_design')`

        * - :need:`wp__verification_sw_unit_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - <Link to issue>
          - Bazel test suites ``//score/kvs:unit_tests`` and ``//score/kvs/rust_kvs:unit_tests``, results in :need:`mod_vrep__persistency__report`
          - :ndf:`copy('status', need_id='mod_vrep__persistency__report')`

        * - :need:`wp__sw_implementation_inspection`
          - :need:`gd_chklst__impl_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__impl_inspection_checklist')`
          - `#321 <https://github.com/eclipse-score/persistency/issues/321>`__
          - :need:`doc__kvs_impl_inspection`
          - :ndf:`copy('status', need_id='doc__kvs_impl_inspection')`

        * - :need:`wp__verification_comp_int_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - <Link to issue>
          - Bazel test suites ``//score/kvs/tests/test_cases:cit_cpp`` and ``//score/kvs/tests/test_cases:cit_rust``, results in :need:`mod_vrep__persistency__report`
          - :ndf:`copy('status', need_id='mod_vrep__persistency__report')`

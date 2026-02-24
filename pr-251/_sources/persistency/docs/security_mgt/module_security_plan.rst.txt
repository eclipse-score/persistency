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

Module Security Plan
********************

.. document:: Persistency Security Plan
   :id: doc__persistency_security_plan
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__module_security_plan
   :tags: persistency

Security Management Context
===========================

This Security Plan adds to the :need:`gd_guidl__security_plan_definitions` all the module development relevant workproducts needed for ISO SAE 21434 conformity.

Security Management Scope
=========================

This Security Plan's scope is a SW module of the SW platform :ref:`module_documentation`.
The module consists of one or more SW components and will be qualified as a EooC.

Security Management Roles
=========================

.. list-table:: Module roles
        :header-rows: 1

        * - Role
          - Assignee

        * - Security Manager
          - TBD

        * - Module Project Manager (= Feature team lead)
          - TBD

Tailoring
=========

Additional to the tailoring in the SW platform project as defined in the :need:`gd_guidl__security_plan_definitions` we define here the additional tailoring on module level.

| - Excluded for this module are additionally the following workproducts (and their related requirements):
|   - No workproducts excluded

Security Module Workproducts
=============================

.. list-table:: Module Workproducts
        :header-rows: 1

        * - Workproduct Id
          - Link to process
          - Process status
          - Link to issue
          - Link to WP
          - WP status

        * - :need:`wp__module_security_plan`
          - :need:`gd_guidl__security_plan_definitions`
          - :ndf:`copy('status', need_id='gd_guidl__security_plan_definitions')`
          - <Link to issue>
          - this document
          - valid

        * - :need:`wp__module_security_package`
          - :need:`gd_guidl__security_package`
          - :ndf:`copy('status', need_id='gd_guidl__security_package')`
          - <Link to issue>
          - this document (including the linked documentation)
          - valid

        * - :need:`wp__fdr_reports` (module Security Plan)
          - :need:`gd_chklst__security_plan`
          - :ndf:`copy('status', need_id='gd_chklst__security_plan')`
          - <Link to issue>
          - :need:`doc__persistency_security_plan_fdr`
          - :ndf:`copy('status', need_id='doc__persistency_security_plan_fdr')`

        * - :need:`wp__fdr_reports` (module Security Package)
          - :need:`gd_chklst__security_package`
          - :ndf:`copy('status', need_id='gd_chklst__security_package')`
          - <Link to issue>
          - :need:`doc__persistency_security_package_fdr`
          - :ndf:`copy('status', need_id='doc__persistency_security_package_fdr')`

        * - :need:`wp__fdr_reports` (module's Security Analyses)
          - :need:`gd_guidl__security_analysis`
          - :ndf:`copy('status', need_id='gd_guidl__security_analysis')`
          - <Link to issue>
          - <Link to WP>
          - <automated>

        * - :need:`wp__audit_report_security`
          - performed by external experts
          - n/a
          - <Link to issue>
          - <Link to WP>
          - <WP status (manual)>

        * - :need:`wp__module_security_manual`
          - :need:`gd_temp__security_manual`
          - :ndf:`copy('status', need_id='gd_temp__security_manual')`
          - <Link to issue>
          - <Link to WP>
          - <automated>

        * - :need:`wp__verification_module_ver_report`
          - :need:`gd_temp__mod_ver_report`
          - :ndf:`copy('status', need_id='gd_temp__mod_ver_report')`
          - <Link to issue>
          - <Link to WP>
          - <automated>

        * - :need:`wp__module_sw_release_note`
          - :need:`gd_temp__rel_mod_rel_note`
          - :ndf:`copy('status', need_id='gd_temp__rel_mod_rel_note')`
          - <Link to issue>
          - <Link to WP>
          - <automated>

        * - :need:`wp__sw_module_sbom`
          - template not yet created
          - not started
          - <Link to issue>
          - <Link to WP>
          - <automated>


.. list-table:: Component Workproducts
        :header-rows: 1

        * - Workproduct Id
          - Link to process
          - Process status
          - Link to issue
          - Link to WP
          - WP status

        * - :need:`wp__requirements_comp`
          - :need:`gd_temp__req_comp_req`
          - :ndf:`copy('status', need_id='gd_temp__req_comp_req')`
          - <Link to issue>
          - :need:`doc__persistency_kvs_requirements`
          - <automated>

        * - :need:`wp__requirements_comp_aou`
          - :need:`gd_temp__req_aou_req`
          - :ndf:`copy('status', need_id='gd_temp__req_aou_req')`
          - <Link to issue>
          - :need:`doc__persistency_kvs_requirements`
          - <automated>

        * - :need:`wp__requirements_inspect`
          - :need:`gd_chklst__req_inspection`
          - :ndf:`copy('status', need_id='gd_chklst__req_inspection')`
          - n/a
          - Checklist used in Pull Request Review
          - n/a

        * - :need:`wp__component_arch`
          - :need:`gd_temp__arch_comp`
          - :ndf:`copy('status', need_id='gd_temp__arch_comp')`
          - <Link to issue>
          - :need:`doc__persistency_kvs_architecture`
          - <automated>

        * - :need:`wp__sw_component_security_analysis`
          - :need:`wp__sw_component_security_analysis`
          - :ndf:`copy('status', need_id='wp__sw_component_security_analysis')`
          - <Link to issue>
          - <Link to WP>
          - <automated>

        * - :need:`wp__sw_arch_verification`
          - :need:`gd_chklst__arch_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__arch_inspection_checklist')`
          - <Link to issue>
          - Checklist used in Pull Request Review
          - <automated>

        * - :need:`wp__sw_implementation`
          - :need:`gd_guidl__implementation`
          - :ndf:`copy('status', need_id='gd_guidl__implementation')`
          - <Link to issue>
          - <Link to WP>
          - <automated>

        * - :need:`wp__verification_sw_unit_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - <Link to issue>
          - <Link to WP>
          - <automated>

        * - :need:`wp__sw_implementation_inspection`
          - :need:`gd_chklst__impl_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__impl_inspection_checklist')`
          - <Link to issue>
          - Checklist used in Pull Request Review
          - <automated>

        * - :need:`wp__verification_comp_int_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - <Link to issue>
          - <Link to WP>
          - <automated>

Special Note
============

Module security plan template will be refined and existing content will be synchronized as per new template.

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

Module Safety Plan
******************

.. document:: Persistency Safety Plan
   :id: doc__persistency_safety_plan
   :status: valid
   :version: 2
   :safety: ASIL_B
   :security: NO
   :realizes: wp__module_safety_plan[version==1]
   :tags: persistency

:note: The module safety plan shall be continuously maintained during the project.
       Deviations to the module safety plan are documented :ref:`here <persistency_safety_package_deviations>`.

Functional Safety Management Context
====================================

This Safety Plan adds to the :need:`wp__platform_safety_plan` all the module development relevant work products needed for ISO 26262 conformity.

Functional Safety Management Scope
==================================

This Safety Plan's scope is a SW module of the SW platform :ref:`persistency_module_documentation`.
The module consists of one or more SW components and will be qualified as a SEooC.

Functional Safety Management Roles
==================================

.. list-table:: Module roles
        :header-rows: 1

        * - Role
          - Assignee

        * - Safety Manager
          - Volker Häussler

        * - Module Project Manager (= Feature team lead)
          - Uwe Maucher

Tailoring
=========

Additional to the tailoring in the SW platform project as defined in the :need:`wp__platform_safety_plan` we define here the additional tailoring on module level.

- Excluded for this module are additionally the following work products (and their related requirements):

  - none

- Notes on the execution of the planned work products:

  - The component safety analyses (:need:`doc__kvs_fmea`, :need:`doc__kvs_dfa`) refer to the feature safety analyses,
    because the KVS component has no internal components (:need:`doc__kvs_component_architecture`).
  - The detailed design is captured in the source code (public API headers and traits, source code comments) according to
    :need:`gd_guidl__implementation`. :need:`doc__kvs_detailed_design` documents the decomposition into units.

Functional Safety Module Work products
======================================

One set of work products for the module and one set for each component of the module:

Module Work products List
-------------------------

.. list-table:: Module Work products
        :header-rows: 1

        * - Work product Id
          - Link to process
          - Process status
          - Link to WP

        * - :need:`wp__module_safety_plan`
          - :need:`gd_guidl__saf_plan_definitions`
          - :ndf:`copy('status', need_id='gd_guidl__saf_plan_definitions')`
          - this document

        * - :need:`wp__module_safety_package`
          - :need:`gd_guidl__saf_package`
          - :ndf:`copy('status', need_id='gd_guidl__saf_package')`
          - this document (including the linked documentation)

        * - :need:`wp__fdr_reports` (module Safety Plan)
          - :need:`gd_chklst__safety_plan`
          - :ndf:`copy('status', need_id='gd_chklst__safety_plan')`
          - :need:`doc__persistency_safety_plan_fdr`

        * - :need:`wp__fdr_reports` (module Safety Package)
          - :need:`gd_chklst__safety_package`
          - :ndf:`copy('status', need_id='gd_chklst__safety_package')`
          - :need:`doc__persistency_safety_package_fdr`

        * - :need:`wp__fdr_reports` (module's Safety Analyses & DFA)
          - :need:`gd_chklst__safety_analysis`
          - :ndf:`copy('status', need_id='gd_chklst__safety_analysis')`
          - :need:`doc__module_safety_analysis_fdr`

        * - :need:`wp__audit_report`
          - performed by external experts
          - n/a
          - Planned with the next S-CORE safety audit, see :ref:`persistency_safety_package_deviations`

        * - :need:`wp__module_safety_manual`
          - :need:`gd_temp__safety_manual`
          - :ndf:`copy('status', need_id='gd_temp__safety_manual')`
          - :need:`doc__persistency_safety_manual`

        * - :need:`wp__verification_module_ver_report`
          - :need:`gd_temp__mod_ver_report`
          - :ndf:`copy('status', need_id='gd_temp__mod_ver_report')`
          - :need:`doc__persistency_verification_report`

        * - :need:`wp__module_sw_release_note`
          - :need:`gd_temp__rel_mod_rel_note`
          - :ndf:`copy('status', need_id='gd_temp__rel_mod_rel_note')`
          - :need:`doc__persistency_release_note`


Component KVS Work products List
--------------------------------

.. list-table:: Component KVS Work products
        :header-rows: 1

        * - Work product Id
          - Link to process
          - Process status
          - Link to WP

        * - :need:`wp__requirements_comp`
          - :need:`gd_temp__req_comp_req`
          - :ndf:`copy('status', need_id='gd_temp__req_comp_req')`
          - :need:`doc__kvs_requirements`

        * - :need:`wp__requirements_comp_aou`
          - :need:`gd_temp__req_aou_req`
          - :ndf:`copy('status', need_id='gd_temp__req_aou_req')`
          - :need:`doc__kvs_requirements`

        * - :need:`wp__requirements_inspect`
          - :need:`gd_chklst__req_inspection`
          - :ndf:`copy('status', need_id='gd_chklst__req_inspection')`
          - :need:`doc__kvs_req_inspection`

        * - :need:`wp__component_arch`
          - :need:`gd_temp__arch_comp`
          - :ndf:`copy('status', need_id='gd_temp__arch_comp')`
          - :need:`doc__kvs_component_architecture`

        * - :need:`wp__sw_arch_verification`
          - :need:`gd_chklst__arch_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__arch_inspection_checklist')`
          - :need:`doc__kvs_arc_inspection`

        * - :need:`wp__sw_component_fmea`
          - :need:`gd_temp__comp_saf_fmea`
          - :ndf:`copy('status', need_id='gd_temp__comp_saf_fmea')`
          - :need:`doc__kvs_fmea`

        * - :need:`wp__sw_component_dfa`
          - :need:`gd_temp__comp_saf_dfa`
          - :ndf:`copy('status', need_id='gd_temp__comp_saf_dfa')`
          - :need:`doc__kvs_dfa`

        * - :need:`wp__sw_implementation`
          - :need:`gd_guidl__implementation`
          - :ndf:`copy('status', need_id='gd_guidl__implementation')`
          - :need:`doc__kvs_detailed_design` & `source code <https://github.com/eclipse-score/persistency/tree/main/score/kvs>`__

        * - :need:`wp__verification_sw_unit_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - Bazel test suites ``//score/kvs:unit_tests`` (C++) and ``//score/kvs/rust_kvs:unit_tests`` (Rust),
            results in :need:`doc__persistency_verification_report`

        * - :need:`wp__sw_implementation_inspection`
          - :need:`gd_chklst__impl_inspection_checklist`
          - :ndf:`copy('status', need_id='gd_chklst__impl_inspection_checklist')`
          - :need:`doc__kvs_impl_inspection`

        * - :need:`wp__verification_comp_int_test`
          - :need:`gd_guidl__verification_guide`
          - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
          - Bazel test suites ``//score/kvs/tests/test_cases:cit_cpp`` and ``//score/kvs/tests/test_cases:cit_rust``,
            results in :need:`doc__persistency_verification_report`

The KVS component is a new development, so no :need:`wp__sw_component_class` is planned for it.




OSS (sub-)component qualification plan
--------------------------------------

The Rust implementation of the KVS component uses the OSS crates ``tinyjson`` (JSON parsing and generation) and
``adler32`` (checksum). The C++ implementation uses the S-CORE baselibs only.

The classification of these OSS crates (:need:`wp__sw_component_class`) and the resulting qualification plan are open,
see :ref:`persistency_safety_package_deviations`.

Link to project planning
------------------------

`Persistency Feature Team Planning <https://github.com/orgs/eclipse-score/projects/20/views/12>`__


Module Safety Package
=====================

To create the safety package (according to :need:`gd_guidl__saf_package`) the following
documents and work products status have to go to "valid" (after the relevant verification were performed).

Module Documents Status
-----------------------

For all the work product documents the status can be seen by following the "Link to WP" and in
:ref:`persistency_module_documentation`.

Component Documents Status
--------------------------

For all the work product documents the status can be seen by following the "Link to WP" and in
:ref:`component_documentation`.


Component Requirements Status
-----------------------------

.. needtable::
   :filter: docname is not None and "kvs" in docname and "requirements" in docname
   :style: table
   :types: comp_req
   :columns: id;status
   :colwidths: 25,25
   :sort: title

Component AoU Status
--------------------

.. needtable::
   :filter: docname is not None and "kvs" in docname and "requirements" in docname
   :style: table
   :types: aou_req
   :columns: id;status
   :colwidths: 25,25
   :sort: title

Component Architecture Status
-----------------------------

.. needtable::
   :filter: docname is not None and "kvs" in docname and "architecture" in docname
   :style: table
   :types: comp_arc_sta; comp_arc_dyn
   :columns: id;status
   :colwidths: 25,25
   :sort: title

.. _persistency_safety_package_deviations:

Deviations from Module Safety Plan
----------------------------------

The following deviations from the module safety plan are present in the module safety package.
These are deviations from planned processes execution and/or work product results,
safety anomalies in the sense of known bugs in the software are reported in the release notes.

.. list-table:: Deviations from the module safety plan
        :header-rows: 1

        * - Work product
          - Deviation
          - Planned resolution

        * - :need:`wp__sw_component_class`
          - The OSS crates ``tinyjson`` and ``adler32`` used by the Rust implementation are not classified. The previous
            classification of Tiny JSON was removed with the change of the architecture to the baselibs JSON interface.
          - Classify the crates or replace them by the baselibs interfaces in the Rust implementation.



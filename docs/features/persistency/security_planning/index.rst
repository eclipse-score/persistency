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


Persistency Security Work Products List
#######################################

.. document:: Persistency KVS Security WPs
   :id: doc__persistency_security_wp
   :status: valid
   :version: 2
   :safety: ASIL_B
   :security: YES
   :realizes: wp__platform_security_plan[version==1]
   :tags: persistency

Tailoring
=========

Additional to the tailoring in the SW platform project as defined in the project's :need:`wp__platform_security_plan` we define here the additional tailoring on feature level.

- Excluded for this feature are additionally the following work products (and their related requirements):

  - none

Security Work Products List
===========================

.. list-table:: Persistency Security Work products
    :header-rows: 1

    * - Work product Id
      - Link to process
      - Process status
      - Link to WP

    * - :need:`wp__feat_request`
      - :need:`gd_temp__change_feature_request`
      - :ndf:`copy('status', need_id='gd_temp__change_feature_request')`
      - :need:`doc__persistency`

    * - :need:`wp__requirements_feat`
      - :need:`gd_temp__req_feat_req`
      - :ndf:`copy('status', need_id='gd_temp__req_feat_req')`
      - :need:`doc__feature_persistency_requirements`

    * - :need:`wp__requirements_feat_aou`
      - :need:`gd_temp__req_aou_req`
      - :ndf:`copy('status', need_id='gd_temp__req_aou_req')`
      - :need:`doc__feature_persistency_requirements`

    * - :need:`wp__feature_arch`
      - :need:`gd_temp__arch_feature`
      - :ndf:`copy('status', need_id='gd_temp__arch_feature')`
      - :need:`doc__persistency_architecture`

    * - :need:`wp__feature_security_analysis`
      - :need:`gd_guidl__security_analysis`
      - :ndf:`copy('status', need_id='gd_guidl__security_analysis')`
      - :need:`doc__persistency_stride`

    * - :need:`wp__requirements_inspect`
      - :need:`gd_chklst__req_inspection`
      - :ndf:`copy('status', need_id='gd_chklst__req_inspection')`
      - :need:`doc__feature_persistency_requirements_chklst`

    * - :need:`wp__sw_arch_verification`
      - :need:`gd_chklst__arch_inspection_checklist`
      - :ndf:`copy('status', need_id='gd_chklst__arch_inspection_checklist')`
      - :need:`doc__persistency_arc_inspection`

    * - :need:`wp__verification_feat_int_test`
      - :need:`gd_guidl__verification_guide`
      - :ndf:`copy('status', need_id='gd_guidl__verification_guide')`
      - Not yet available

Feature Security Package
========================

To create the security package (according to :need:`gd_guidl__security_package`) the following
documents and work products status have to go to "valid" (after the relevant verification were performed).

Feature Documents Status
------------------------

For all the work product documents the status can be seen by following the "Link to WP" and in
:ref:`persistency_module_documentation`.

Feature Requirements Status
---------------------------

The feature requirements are maintained in the SCORE platform repository.

.. needtable::
   :filter: type == "feat_req" and id.startswith("feat_req__persistency__") and security == "YES"
   :style: table
   :columns: id;status
   :colwidths: 25,25
   :sort: title

Feature AoU Status
------------------

.. needtable::
   :filter: type == "aou_req" and id.startswith("aou_req__persistency__") and security == "YES"
   :style: table
   :columns: id;status
   :colwidths: 25,25
   :sort: title

Feature Architecture Status
---------------------------

.. needtable::
   :filter: docname is not None and "persistency" in docname and "architecture" in docname and security == "YES"
   :style: table
   :types: feat_arc_sta; feat_arc_dyn
   :columns: id;status
   :colwidths: 25,25
   :sort: title

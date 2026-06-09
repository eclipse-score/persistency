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
   :status: valid
   :safety: ASIL_B
   :security: YES
   :realizes: wp__requirements_inspect

Requirement Inspection Checklist
================================

Purpose
-------

The purpose of this requirement inspection checklist is to collect the topics to be checked during requirements inspection.

Conduct
-------

As described in the concept :need:`doc_concept__wp_inspections` the following "inspection roles" are expected to be filled:

- content responsible (author): `<https://github.com/a-zw>`_
- reviewer: `<https://github.com/aschemmel-tech>`_, `<https://github.com/umaucher>`_
- moderator: `<https://github.com/PandaeDo>`_
- test expert: <one of the reviewers explicitly named here, to cover REQ_08_01 as described>

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
      - YES
      - All reqs use the generic term "The component shall ..."
      -
    * - REQ_02_01
      - Is the requirement description *comprehensible* ?
      - If you think the requirement is hard to understand, comment here.
      - NO
      - - Several requirements use the term "snapshot" which is not explained.
        - In :need:`comp_req__kvs__default_value_types` it is unclear what is the meaning of "only permitted"
        - In :need:`comp_req__kvs__pers_data_schema` it is not clear what downgrade means (it is also not required from the feature req). And it seems that really the application/user is responsible for thee versioning?
        - In :need:`comp_req__kvs__field_mode` - access to what?
      - issue link TBD
    * - REQ_02_02
      - Is the requirement description *unambiguous* ?
      - Especially search for "weak words" like "about", "etc.", "relevant" and others (see the internet documentation on this). This check shall be supported by tooling.
      - NO
      - :need:`comp_req__kvs__field_mode` uses hard to verify term "as much as possible"
      - issue link TBD
    * - REQ_02_03
      - Is the requirement description *atomic* ?
      - A good way to think about this is to consider if the requirement may be tested by one (positive) test case or needs more of these. The requirement formulation template should also avoid being non-atomic already. Note that there are cases where also non-atomic requirements are the better ones, for example if those are better understandable.
      - YES
      - Several requirements use two "shall" which may indicate non atomic content, but the activities described like this are closely related so it is ok.
      -
    * - REQ_02_04
      - Is the requirement description *feasible* ?
      - If at the time of the inspection the requirement has already some implementation, the answer is yes. This can be checked via traces, but also :need:`gd_req__req_attr_impl` shows this. In case the requirement has no implementation at the time of inspection (i.e. not implemented at least as "proof-of-concept"), a development expert should be invited to the Pull-Request review to explicitly check this item.
      - YES
      - Requirements are implemented already.
      -
    * - REQ_02_05
      - Is the requirement description *independent from implementation* ?
      - This checkpoint should improve requirements definition in the sense that the "what" is described and not the "how" - the latter should be described in architecture/design derived from the requirement. But there can also be a good reason for this, for example we would require using a file format like JSON and even specify the formatting standard already on stakeholder requirement level because we want to be compatible. A finding in this checkpoint does not mean there is a safety problem in the requirement.
      - YES
      - The level of the requirements is component level, so some implementation detail is to be expected, but it seems not too detailed.
      -
    * - REQ_03_01
      - Is the *linkage to the parent requirement* correct?
      - Linkage to correct levels and ASIL attributes is checked automatically, but it needs checking if the child requirement implements (at least) a part of the parent requirement.
      - NO
      - - :need:`comp_req__kvs__default_value_query` does not implement a part of its linked :need:`feat_req__persistency__default_value_file`
        - :need:`comp_req__kvs__default_value_cfg` does not implement a part of its linked :need:`feat_req__persistency__reset_to_default` and :need:`feat_req__persistency__default_value_get`
        - :need:`comp_req__kvs__default_val_chksum` should only link to :need:`feat_req__persistency__default_value_file`
        - stopped the inspection at this point - too many findings. Request rework of complete requirement set.
      - issue link TBD
    * - REQ_04_01
      - Is the requirement *internally and externally consistent*?
      - Does the requirement contradict other requirements within the same or higher levels? One may restrict the search to the feature for component requirements, for features to other features using same components. Is the description of the requirement consistent with all its attributes (if not already part of another check, e.g. does the title fit?).
      - NO
      - - Requirement :need:`comp_req__kvs__value_serialize` speaks about serialization/deserialization of JSON, it is expected that this is a requirement for a "JSON" component and not for KVS
        - All requirement IDs are using "persistency" as a component name, it should be "persistency_kvs" according to :need:`comp__persistency_kvs`
      - issue link TBD
    * - REQ_05_01
      - Do the software requirements consider *timing constraints*?
      - This checkpoint encourages to think about timing constraints even if those are not explicitly mentioned in the parent requirement. If the reviewer of a requirement already knows or suspects that the code execution will be consuming a lot of time, one should think of the expectation of a "user".
      - YES
      - Timing consideration is for example given in :need:`feat_req__persistency__async_api`.
      -
    * - REQ_06_01
      - Does the requirement consider *external interfaces*?
      - The SW platform's external interfaces (to the user) are defined in the Feature Architecture, so the Feature and Component Requirements should determine the input data use and setting of output data for these interfaces. Are all output values defined?
      - NO
      - It is not completely defined how the operations in logic_arc_int__persistency__interface are used, for example "open" and "flush".
      - issue link TBD
    * - REQ_07_01
      - Is the *safety* attribute set correctly?
      - Derived requirements are checked automatically, see :need:`gd_req__req_linkage_safety`. But for the top level requirements (and also all AoU) this needs to be checked manually for correctness.
      - YES
      - automated check
      -
    * - REQ_07_02
      - Is the attribute *security* set correctly?
      - For component requirements this checklist item is supported by automated check: "Every requirement which satisfies a feature requirement with security attribute set to YES inherits this". But the component requirements/architecture may additionally also be subject to a :need:`wp__sw_component_security_analysis`.
      - YES
      - automated check
      -
    * - REQ_08_01
      - Is the requirement *verifiable*?
      - If at the time of the inspection already tests are created for the requirement, the answer is yes. This can be checked via traces, but also :need:`gd_req__req_attr_test_covered` shows this. In case the requirement is not sufficiently traced to test cases already, a test expert is invited to the inspection to give their opinion whether the requirement is formulated in a way that supports test development and the available test infrastructure is sufficient to perform the test.
      - YES
      - Reqs either already have a testlink or seem to easily testable. With the following exceptions (which need to be verified according to REQ_08_02):

        - :need:`comp_req__kvs__pers_data_version` - absence of a feature cannot be tested
        - :need:`comp_req__kvs__field_mode` - as much as possible?
        - :need:`comp_req__kvs__permission_control` - non-implementation of a feature cannot be tested
      -
    * - REQ_08_02
      - Is the requirement verifiable by design or code review in case it is not feasibly testable?
      - In very rare cases a requirement may not be verifiable by test cases, for example a specific non-functional requirement. In this case a requirement analysis verifies the requirement by design/code review. If such a requirement is in scope of this inspection, please check this here and link to the respective review record. A test expert is invited to the inspection to confirm their opinion that the requirement is not testable.
      - NO
      - Requirements mentioned in REQ_08_01 were not confirmed by design/code review to date.
      - issue link TBD
    * - REQ_09_01
      - Do the requirements that define a safety mechanism specify the error reaction leading to a safe state?
      - Alternatively to the safe state there could also be "repair" mechanisms. Also do not forget to consider REQ_05_01 for these.
      - NO
      - - :need:`comp_req__kvs__default_val_chksum` - defines safety mechanism, no error reaction defined
        - :need:`comp_req__kvs__pers_data_csum` - defines part of safety mechanism, error reaction not needed
        - :need:`comp_req__kvs__pers_data_csum_vrfy` defines part of safety mechanism, no error reaction defined
      - issue link TBD

The following requirements in "valid" state and with "inspected" tag set are in the scope of this inspection:

.. needtable::
   :filter: docname is not None and "component_name" in docname and "requirements" in docname and status == "valid"
   :style: table
   :types: comp_req
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title


And also the following AoUs in "valid" state and with "inspected" tag set (for these please answer the questions above as if the AoUs are requirements, except question REQ_03_01):

.. needtable::
   :filter: docname is not None and "component_name" in docname and "requirements" in docname and status == "valid"
   :style: table
   :types: aou_req
   :columns: id;status;tags
   :colwidths: 25,25,25
   :sort: title

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

Persistency Safety Plan Formal Review Report
============================================

.. document:: Persistency Safety Plan Formal Review
   :id: doc__persistency_safety_plan_fdr
   :status: draft
   :version: 2
   :safety: ASIL_B
   :security: NO
   :realizes: wp__fdr_reports[version==1]
   :tags: persistency

**Purpose**

The purpose of this safety plan formal review checklist is to report status of the review for the safety plan.

**Conduct**
As described in :need:`wf__p_formal_rv`, the formal document review is performed by an "external" safety manager:

- reviewer: Alexander Schemmel (`<https://github.com/aschemmel-tech>`_)
- scope: the module safety plan :need:`doc__persistency_safety_plan` (version 2) including the feature safety
  work products list :need:`doc__persistency_safety_wp` and the skill check :need:`doc__persistency_codeowners`

**Checklist**

See also :need:`doc_concept__wp_inspections` for further information about reviews in general and inspection in particular.

.. list-table:: Safety Plan Checklist
        :header-rows: 1

        * - Id
          - Safety plan activity
          - Compliant to ISO 26262?
          - Reference
          - Comment

        * - 1
          - Is the rationale for the safety work products tailoring included?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6451>`
            :need:`[[title]] <std_req__iso26262__management_6455>`
            :need:`[[title]] <std_req__iso26262__management_6457>`
            :need:`[[title]] <std_req__iso26262__management_6467>`
          - The module and the feature add no further exclusions to the tailoring of the :need:`wp__platform_safety_plan`
            ("none" in the module plan, "no tailoring" in :need:`doc__persistency_safety_wp`).
            The notes on the execution give the rationale why the component FMEA/DFA refer to the feature analyses
            (KVS has no internal components) and where the detailed design is captured (source code and :need:`doc__kvs_detailed_design`).

        * - 2
          - Is impact analysis planned in case of re-use of SW (needed for every release following the first formal release)?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6452>`
          - The module safety plan adds to the :need:`wp__platform_safety_plan`, whose tailoring defines that the safety
            planning for a change request on an existing feature is done based on an impact analysis.
            The module safety plan does not repeat this.

        * - 3
          - Does the safety plan define all needed activities for safety management (incl. formal document review and Safety Audit)?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6465>`
            :need:`[[title]] <std_req__iso26262__management_6491>`
            :need:`[[title]] <std_req__iso26262__management_64111>`
          - Planned are the formal document reviews of the safety plan, the safety package and the safety analyses
            (:need:`wp__fdr_reports`), the safety manual, the module verification report and the release note.
            The safety audit (:need:`wp__audit_report`) is not performed within S-CORE ("n/a" in the work product list);
            the module prepares the work products, the audit is commissioned by the integrator or user if needed.

        * - 4
          - Does the safety plan define all needed activities for System and SW development, integration and verification?
          - NO
          - :need:`[[title]] <std_req__iso26262__management_6465>`
          - Component KVS: requirements, AoU, requirements inspection, architecture, architecture inspection, implementation,
            unit test, implementation inspection and component integration test are planned.
            Feature Persistency (:need:`doc__persistency_safety_wp`): feature request, requirements, AoU, architecture, FMEA, DFA
            and the inspections are planned. Module: safety manual, user manual (:need:`wp__module_user_manual`), module
            verification report and release note are planned. Finding:

            - :need:`wp__verification_feat_int_test` has no link to a work product ("<WP Link>").

        * - 5
          - Does the safety plan define all needed activities for safety analysis and DFA?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6465>`
          - Planned are the feature FMEA and DFA (:need:`doc__persistency_safety_wp`), the component FMEA and DFA
            and their formal document review (:need:`wp__fdr_reports`).

        * - 6
          - Does the safety plan define all needed activities for supporting processes (incl. tool mgt)?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6465>`
          - The supporting processes (configuration, change, documentation, problem resolution and tool management)
            are planned on platform level in the :need:`wp__platform_safety_plan`. The module uses the platform tools
            (Bazel, docs-as-code, GitHub) and adds no module specific tools.

        * - 7
          - Does the safety plan document a responsible for all activities?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6463>`
          - Safety Manager and Module Project Manager are named in the module roles. The work products are
            guarded by the CODEOWNERS whose skills are checked in :need:`doc__persistency_codeowners`; the responsibility
            of the process roles per work product is defined in the processes as stated in the :need:`wp__platform_safety_plan`.

        * - 8
          - If OSS software components is used, is it planned to be qualified?
          - NO
          - :need:`[[title]] <std_req__iso26262__management_6455>`
          - The Rust implementation uses the OSS crates ``tinyjson`` and ``adler32``. Their classification
            (:need:`wp__sw_component_class`) and qualification plan are open and documented as deviation in
            :ref:`persistency_safety_package_deviations`. Replacement of ``adler32`` is discussed in
            `#413 <https://github.com/eclipse-score/persistency/issues/413>`_.

        * - 9
          - Is a safety manager and a project manager appointed for the project?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6461>`
            :need:`[[title]] <std_req__iso26262__management_6462>`
          - Safety Manager: Volker Häussler, Module Project Manager (= Feature team lead): Uwe Maucher.

        * - 10
          - Is safety plan sufficiently linked to the project plan?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6464>`
          - The module safety plan links to the
            `Persistency Feature Team Planning <https://github.com/orgs/eclipse-score/projects/20/views/12>`__.

        * - 11
          - Is safety plan updated iteratively to show the progress?
          - YES
          - :need:`[[title]] <std_req__iso26262__management_6468>`
          - The safety plan is in version 2 and maintained via pull requests (history in the repository); the work product
            tables show the process status, the linked documents show the work product status.

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

Safety Manual
=============

.. document:: Persistency Safety Manual
   :id: doc__persistency_safety_manual
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :tags: persistency
   :realizes: wp__module_safety_manual[version==1]

Introduction/Scope
------------------
| This manual covers the module Persistency with the feature Persistency, which is realized by the component KVS (:need:`comp__persistency_kvs`) in a C++ and a Rust implementation.

Assumed Platform Safety Requirements
------------------------------------
| For the module persistency the following safety related stakeholder requirements are assumed to define the top level functionality (purpose) of the module persistency. I.e. from these all the feature and component requirements implemented are derived.
| List of stakeholder requirements, with ASIL B, the module's components requirements are derived from.

.. needtable::
   :style: table
   :columns: title;id;status
   :colwidths: 25,25,15
   :sort: title

   results = []
   stkh_ids = set()

   for need in needs.filter_types(["feat_req"]):
      if need["id"].startswith("feat_req__persistency__"):
         for link in need["derived_from"]:
            stkh_ids.add(link.split("[")[0])

   for need in needs.filter_types(["stkh_req"]):
      if need["id"] in stkh_ids and need["safety"] == "ASIL_B":
         results.append(need)


Assumptions of Use
------------------


AoU Requirements
################

.. aou_req:: Persistency Error handling
   :id: aou_req__persistency__error_handling
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 2
   :tags: persistency, environment

   The application shall detect and handle the unavailability of the feature persistency.
   Unavailability covers errors reported by the persistency API as well as persistency calls which do
   not return or return too late (e.g. caused by blocked or delayed execution of the calling context).

Assumptions on the Environment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
| Generally the assumption of the S-CORE platform SEooC is that it is integrated in a safe system, i.e. the POSIX OS it runs on is qualified and also the HW related failures are taken into account by the system integrator, if not otherwise stated in the module's safety concept.
| The module expects the following OS services to be safe:
| - file system access: open, read, write, flush and sync to the storage (``fdatasync``), rename and remove of files. The C++ implementation uses them via ``score::filesystem`` of the baselibs and the C standard library, the Rust implementation via the Rust standard library (``std::fs``);
| - thread synchronization (mutex);
| - heap memory allocation.

List of AoUs expected from the environment the module runs on:

.. needtable::
   :style: table
   :columns: title;id;status
   :colwidths: 25,25,25
   :sort: title

   results = []

   for need in needs.filter_types(["aou_req"]):
      if need and "persistency" in need["tags"]:
         if need and "environment" in need["tags"]:
                results.append(need)

Assumptions on the User
^^^^^^^^^^^^^^^^^^^^^^^
| As there is no assumption on which specific OS and HW is used, the integration testing of the stakeholder and feature requirements is expected to be performed by the user of the platform SEooC. Tests covering all stakeholder and feature requirements performed on a reference platform (tbd link to reference platform specification), reviewed and passed are included in the platform SEooC safety package.
| Additionally the components of the platform may have additional specific assumptions how they are used. These are part of every module documentation: :ref:`persistency_module_documentation`. Assumptions from components to their users can be fulfilled in two ways:
| 1. There are assumption which need to be fulfilled by all SW components, e.g. "every user of an IPC mechanism needs to make sure that he provides correct data (including appropriate ASIL level)" - in this case the AoU is marked as "platform".
| 2. There are assumption which can be fulfilled by a safety mechanism realized by some other S-CORE platform component and are therefore not relevant for an user who uses the whole platform. But those are relevant if you chose to use the module SEooC stand-alone - in this case the AoU is marked as "module". An example would be the "JSON read" which requires "The user shall provide a string as input which is not corrupted due to HW or QM SW errors." - which is covered when using together with safe S-CORE platform persistency feature.

List of AoUs on the user of the platform features or the module of this safety manual:

Note: The platform safety manual collects all platform wide AoUs (to be fulfilled by the user for any feature).
This module safety manual collects all AoUs specific to the feature Persistency and its realizing component.
For the feature Persistency the platform safety manual and this module safety manual have to be considered.

.. needtable::
   :style: table
   :columns: title;id;status
   :colwidths: 25,25,25
   :sort: title

   results = []

   for need in needs.filter_types(["aou_req"]):
      if need and "environment" not in need["tags"]:
         if need and "persistency" in need["tags"]:
                results.append(need)

Safety concept of the SEooC
---------------------------
| Persistency is developed fully deterministic. Detected errors are reported to the application, which has to handle them.
| Persistency is executed in the execution context of the calling application. Failures which can not be detected by persistency
| itself, like blocked or delayed execution, result in persistency not being available (no or too late response). The application
| has to handle this unavailability (see :need:`aou_req__persistency__error_handling`). Keeping persistency available is therefore
| not a safety relevant assumption on the application.
| The integrity of the persisted data is checked when it is loaded: the data file is only used if its Adler-32 checksum
| matches the stored checksum file, otherwise an error is reported and the data is not provided (see :need:`feat_req__persistency__integrity_check`).
| The previous states of the data are kept as snapshots, which the application can restore after a reported error.
| The safety analyses of the feature are documented in :need:`doc__persistency_fmea` and :need:`doc__persistency_dfa`.

Safety Anomalies
----------------
| Anomalies (bugs in ASIL SW, detected by testing or by users, which could not be fixed) known before release are documented in the module release notes :need:`doc__persistency_release_note`.
| No known safety anomalies related to the module persistency exist.

References
----------
| Module user manual: :need:`doc__persistency_user_manual`
| Module safety plan: :need:`doc__persistency_safety_plan`
| Feature architecture: :need:`doc__persistency_kvs_architecture`
| Component architecture: :need:`doc__kvs_component_architecture`
| Feature safety analyses: :need:`doc__persistency_fmea`, :need:`doc__persistency_dfa`

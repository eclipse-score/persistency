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

Security Manual
===============

.. document:: Persistency Security Manual
   :id: doc__persistency_security_manual
   :status: valid
   :version: 2
   :safety: ASIL_B
   :security: YES
   :realizes: wp__module_security_manual[version==1]
   :tags: persistency

Introduction/Scope
------------------
| This manual covers the module Persistency with the feature Persistency, which is realized by the component KVS (:need:`comp__persistency_kvs`) in a C++ and a Rust implementation.
| The module is developed as Out of Context (OoC). This manual defines the requirements for the secure integration and usage of the module.

Assumed Platform Security Requirements
--------------------------------------
| For the module Persistency, the following stakeholder requirements are assumed to define the top level functionality (purpose) of the Persistency module. I.e. from these all the feature and component requirements implemented are derived.
| List of stakeholder requirements from which the security relevant feature requirements of Persistency are derived:

.. needtable::
   :style: table
   :columns: title;id;status
   :colwidths: 25,25,15
   :sort: title

   results = []
   stkh_ids = set()

   for need in needs.filter_types(["feat_req"]):
      if need["id"].startswith("feat_req__persistency__") and need["security"] == "YES":
         for link in need["derived_from"]:
            stkh_ids.add(link.split("[")[0])

   for need in needs.filter_types(["stkh_req"]):
      if need["id"] in stkh_ids:
         results.append(need)

| Security relevant feature requirements of Persistency:

.. needtable::
   :filter: type == "feat_req" and id.startswith("feat_req__persistency__") and security == "YES"
   :style: table
   :columns: title;id;status
   :colwidths: 25,25,15
   :sort: title

Assumptions of Use
------------------

AoU Requirements
################

.. aou_req:: Storage Access Rights
   :id: aou_req__persistency__storage_access_rights
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: persistency, environment

   The system integrator shall configure the OS users and groups of the applications and the access rights of the
   KVS working directories such that only the application owning a KVS instance (and, where required, the members of
   its group) can read or write the storage files, snapshot files and default-values files of this KVS instance.

.. aou_req:: Read-only Default Values
   :id: aou_req__persistency__defaults_read_only
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: persistency, environment

   The system integrator shall deploy the default-values files read-only for the applications, so that the default
   values can not be modified at runtime.

.. aou_req:: Confidentiality of Persisted Data
   :id: aou_req__persistency__confidentiality
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: persistency, environment

   The system integrator shall provide confidentiality of persisted data which requires it by the storage
   environment (an encrypted file system or an encrypted storage medium), as the component stores the key-value
   pairs unencrypted.

.. aou_req:: No Protection against Intentional Manipulation
   :id: aou_req__persistency__manipulation_protection
   :reqtype: Non-Functional
   :security: YES
   :safety: QM
   :status: valid
   :version: 1
   :tags: persistency

   The application shall not rely on the integrity check of the component to detect intentional manipulation of
   persisted data. If protection against intentional manipulation is required, the application shall
   authenticate the data itself or require an integrity protected storage from the system integrator.

Assumptions on the Environment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
| The platform and its components are developed as Out of Context (OoC) with assumptions on the environment.
  It is assumed that the platform/components are integrated in a secure system, i.e. qualified POSIX OS.
  Also the HW related failures are taken into account by the system integrator, if not otherwise stated in the module's security concept.
| The module expects the following OS services to be secure:
| - file system access: open, read, write, flush and sync to the storage (``fdatasync``), rename and remove of files, creation of directories. The C++ implementation uses them via ``score::filesystem`` of the baselibs and the C standard library, the Rust implementation via the Rust standard library (``std::fs``);
| - enforcement of the file system access rights (user, group and permission bits) for these calls;
| - process isolation, i.e. the memory of the application using Persistency can not be read or modified by other processes;
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
      if need and "persistency" in need["tags"] and "environment" in need["tags"]:
         if need["security"] == "YES":
            results.append(need)

Assumptions on the User
^^^^^^^^^^^^^^^^^^^^^^^
| As there is no assumption on which specific OS and HW is used, the integration testing of the stakeholder and feature requirements is expected to be performed by the user of the platform OoC. Tests covering all stakeholder and feature requirements performed on a reference platform (tbd link to reference platform specification), reviewed and passed are included in the platform OoC security package.
| Additionally the components of the platform may have additional specific assumptions how they are used. These are part of every module documentation: :ref:`persistency_module_documentation`. Assumptions from components to their users can be fulfilled in two ways:
| 1. There are assumption which need to be fulfilled by all SW components, e.g. "every user of an IPC mechanism needs to make sure that he provides correct data (e.g. including appropriate security (access) control)" - in this case the AoU is marked as "platform".
| 2. There are assumption which can be fulfilled by a security control realized by some other S-CORE platform component and are therefore not relevant for an user who uses the whole platform. But those are relevant if you chose to use the module OoC stand-alone - in this case the AoU is marked as "module".

List of AoUs on the user of the platform features or the module of this Security Manual:

Note: The platform security manual collects all platform wide AoUs (to be fulfilled by the user for any feature).
This module security manual collects all security AoUs specific to the feature Persistency and its realizing component.
For the feature Persistency the platform security manual and this module security manual have to be considered.

.. needtable::
   :style: table
   :columns: title;id;status
   :colwidths: 25,25,25
   :sort: title

   results = []

   for need in needs.filter_types(["aou_req"]):
      if need and "persistency" in need["tags"] and "environment" not in need["tags"]:
         if need["security"] == "YES":
            results.append(need)

Security concept of the OoC
---------------------------
| Persistency is a library which is executed in the process and with the OS user of the calling application. It has no own
  process, no inter-process communication and no network interface. Its attack surface consists of its API, used by
  the application, and of the files it stores in and loads from the file system (storage, snapshot and
  default-values files, see :need:`doc__persistency_kvs_architecture`).
| Access control: the component does not implement own access controls, but relies on the access rights of the file
  system (:need:`comp_req__kvs__permission_control`). Access and permission errors of the file system are reported to the
  application (:need:`comp_req__kvs__permission_err_hndl`). The separation of the data stores of different applications
  (:need:`feat_req__persistency__access_control`) is therefore achieved by the system integrator
  (:need:`aou_req__persistency__storage_access_rights`, :need:`aou_req__persistency__defaults_read_only`).
| Integrity: when a storage file is loaded, its Adler-32 checksum is compared with the stored checksum. Corrupted
  or interrupted storage is detected, reported and not loaded (:need:`comp_req__kvs__corruption_detect_open`,
  :need:`comp_req__kvs__open_need_kvs_behavior`). Adler-32 is not a cryptographic checksum, i.e. it detects accidental
  corruption, but not intentional manipulation by an attacker with write access to the storage
  (:need:`aou_req__persistency__manipulation_protection`).
| Availability: flush operations keep the previously persisted consistent data until the new data is completely
  written (:need:`comp_req__kvs__reset_resistant_write`), previous states are kept as snapshots, whose number is limited
  (:need:`comp_req__kvs__snapshot_max_num_cfg`). Keys are limited in length and character set
  (:need:`comp_req__kvs__key_length`, :need:`comp_req__kvs__key_naming`). Input of malformed storage files is rejected
  by the JSON parser and reported as error.
| Confidentiality: the component stores the key-value pairs unencrypted. The confidential storage
  (:need:`feat_req__persistency__confidential_storage`) is not realized by the component, it has to be provided by the
  storage environment (:need:`aou_req__persistency__confidentiality`).
| Tooling: the KVS tool (``//score/kvs/rust_kvs_tool:kvs_tool``) reads and modifies KVS storage for development and
  test purposes. It has no special rights and can only access the KVS storage accessible to the OS user running it,
  which is restricted by :need:`aou_req__persistency__storage_access_rights`. It is recommended not to deploy the KVS
  tool in production software images, to keep the attack surface small.
| The security analysis of the feature is documented in :need:`doc__persistency_stride`.

.. _persistency_sbom:

Software Bill of Material (SBOM)
--------------------------------
| The SBOM of the module (:need:`wp__sw_module_sbom`) is generated by the Bazel target ``//:sbom_product`` in the
  formats SPDX 2.3 (``sbom_product.spdx.json``) and CycloneDX 1.6 (``sbom_product.cdx.json``). It lists the dependencies
  of the product targets ``//score/kvs:kvs`` (C++) and ``//score/kvs/rust_kvs:rust_kvs`` (Rust) with version, license
  and supplier. The SBOM is generated on demand with ``bazel build //:sbom_product`` and is not yet part of the
  release workflow.
| Build-time, test and documentation tooling is not part of the product and is listed separately by the Bazel target
  ``//:sbom_docs_tests``.

Security Weaknesses, Vulnerabilities
------------------------------------
| Weaknesses, vulnerabilities (bugs in security relevant SW, detected by testing or by users, which could not be fixed) known before release are documented in the module release notes :need:`doc__persistency_release_note`.
| Open security topics at the time of this manual version:
| - :need:`comp_req__kvs__storage_file_permissions` (storage files created with permissions ``640``) is not yet implemented, the storage files are created with the default permissions of the process (``umask``), see `#412 <https://github.com/eclipse-score/persistency/issues/412>`__. Until then :need:`aou_req__persistency__storage_access_rights` has to be fulfilled via the access rights of the KVS working directory.
| - :need:`comp_req__kvs__default_storage_permissions` (default-values file generated by the KVS tool with permissions ``440``) is not yet implemented, the KVS tool does not yet handle default-values files, see `#505 <https://github.com/eclipse-score/persistency/issues/505>`__. Until then :need:`aou_req__persistency__defaults_read_only` has to be fulfilled during deployment.

References
----------
| Module user manual: :need:`doc__persistency_user_manual`
| Module security plan: :need:`doc__persistency_security_plan`
| Feature security analysis: :need:`doc__persistency_stride`
| Feature architecture: :need:`doc__persistency_kvs_architecture`
| Component architecture: :need:`doc__kvs_component_architecture`
| Component requirements: :need:`doc__kvs_requirements`

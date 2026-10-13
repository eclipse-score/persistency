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

.. _persistency_user_manual:

Persistency User Manual
=======================

.. document:: Persistency User Manual
   :id: doc__persistency_user_manual
   :status: valid
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__training_path[version==1]
   :tags: persistency

Overview
========

This user manual guides an application developer through the integration and usage of the Persistency module.
It does not repeat information that is maintained elsewhere. Each section refers to the single source of the
information, either by a link or by a table that is generated from the requirements and architecture of the module.

* Feature and its architecture: :need:`feat__persistency`, :need:`doc__persistency_kvs_architecture`
* Component: :need:`comp__persistency_kvs`, :need:`doc__kvs_requirements`, :need:`doc__kvs_component_architecture`
* Building and testing the module itself: :ref:`persistency_module_documentation`

Environment Needs
=================

The supported languages and operating systems are defined by the following feature requirements:

.. needtable::
   :style: table
   :columns: title;id;status
   :colwidths: 50,35,15
   :filter: id in ["feat_req__persistency__cpp_rust", "feat_req__persistency__os_agnostic"]

The build environment is defined in the repository root:

* `MODULE.bazel <https://github.com/eclipse-score/persistency/blob/main/MODULE.bazel>`__: dependencies,
  toolchains and their versions
* `.bazelversion <https://github.com/eclipse-score/persistency/blob/main/.bazelversion>`__: Bazel version
* `.bazelrc <https://github.com/eclipse-score/persistency/blob/main/.bazelrc>`__: build flags and the target
  platform configurations (``--config=per-...``)

Dependencies
------------

The dependencies, their versions and licenses are listed in the software bill of materials, which is generated
by the Bazel target ``//:sbom_product`` from ``MODULE.bazel.lock`` and ``Cargo.lock``.

Module Configuration Details
============================

The configuration options of a KVS instance (instance ID, default values, storage directory, maximum number of
snapshots) are set via the builder of the instance (``KvsBuilder`` in C++ and Rust). Their parameters and default
values are documented in the API documentation (see `API Reference`_).

The configuration behavior is specified by the following component requirements:

..
   The valid component requirements are grouped by the first entry of "derived_from" (sorted alphabetically
   by sphinx-needs). Each component requirement is shown in exactly one section of this manual. A new
   feature requirement has to be added to the list of one of the sections.

.. needextract::
   :filter: type == "comp_req" and status == "valid" and len(derived_from) > 0 and derived_from[0].split("[")[0] in ["feat_req__persistency__cfg", "feat_req__persistency__default_values", "feat_req__persistency__default_value_get", "feat_req__persistency__reset_to_default", "feat_req__persistency__default_value_file", "feat_req__persistency__dev_mode", "feat_req__persistency__prod_mode"]

Configuration Effects
---------------------

Storage of the data, its integrity check and the snapshots are specified by the following component requirements:

.. needextract::
   :filter: type == "comp_req" and status == "valid" and len(derived_from) > 0 and derived_from[0].split("[")[0] in ["feat_req__persistency__store_data", "feat_req__persistency__load_data", "feat_req__persistency__integrity_check", "feat_req__persistency__versioning", "feat_req__persistency__update_mechanism", "feat_req__persistency__snapshot_create", "feat_req__persistency__snapshot_restore", "feat_req__persistency__snapshot_remove"]

The sequence of the operations (e.g. reading from and flushing to the storage, restoring a snapshot) is shown in
the dynamic architecture of the feature:

.. needtable::
   :style: table
   :columns: title;id
   :colwidths: 60,40
   :filter: type == "feat_arc_dyn" and id.startswith("feat_arc_dyn__persistency__")
   :sort: title

Examples
========

The examples are maintained together with the code:

* `examples/README.md <https://github.com/eclipse-score/persistency/blob/main/examples/README.md>`__:
  integration into a Bazel project and usage of the C++ and Rust implementation
* `score/kvs/rust_kvs/examples <https://github.com/eclipse-score/persistency/tree/main/score/kvs/rust_kvs/examples>`__:
  executable Rust examples
* `score/kvs/tests/test_scenarios <https://github.com/eclipse-score/persistency/tree/main/score/kvs/tests/test_scenarios>`__:
  test scenarios of the C++ and Rust implementation

API Reference
=============

The logical interface of the feature is :need:`logic_arc_int__persistency__interface`. The API of the
implementations is documented in the source code:

* C++: header files in `score/kvs <https://github.com/eclipse-score/persistency/tree/main/score/kvs>`__
* Rust: crate ``rust_kvs`` in `score/kvs/rust_kvs <https://github.com/eclipse-score/persistency/tree/main/score/kvs/rust_kvs>`__,
  documentation generated with ``cargo doc -p rust_kvs``

Keys, values and the API are specified by the following component requirements:

.. needextract::
   :filter: type == "comp_req" and status == "valid" and len(derived_from) > 0 and derived_from[0].split("[")[0] in ["feat_req__persistency__support_datatype_keys", "feat_req__persistency__support_datatype_value", "feat_req__persistency__async_api", "feat_req__persistency__async_completion"]

Performance Considerations
==========================

The performance related properties are defined by the following feature requirements:

.. needtable::
   :style: table
   :columns: title;id;status
   :colwidths: 50,35,15
   :filter: id in ["feat_req__persistency__fast_access", "feat_req__persistency__cached_access", "feat_req__persistency__write_amplification", "feat_req__persistency__dynamic_memory_alloc"]

The verification results are documented in :need:`doc__persistency_verification_report`.

Integration Guidelines
======================

The integration into a Bazel project (module dependency, build flags and library targets) is described in
`examples/README.md <https://github.com/eclipse-score/persistency/blob/main/examples/README.md>`__. The released
versions of the module are listed in the
`S-CORE Bazel registry <https://github.com/eclipse-score/bazel_registry/tree/main/modules/score_persistency>`__.

Concurrent access, multiple instances and access permissions are specified by the following component
requirements:

.. needextract::
   :filter: type == "comp_req" and status == "valid" and len(derived_from) > 0 and derived_from[0].split("[")[0] in ["feat_req__persistency__concurrency", "feat_req__persistency__multiple_kvs", "feat_req__persistency__access_control"]

The application shall fulfill the assumptions of use of the module:

.. needtable::
   :style: table
   :columns: title;id;status
   :colwidths: 50,35,15
   :filter: type == "aou_req" and "persistency" in tags
   :sort: title

Version History, Compatibility, and Troubleshooting
===================================================

For version history, compatibility notes, known issues and security vulnerabilities, refer to
:need:`doc__persistency_release_note`.

Safety and Security
===================

**Safety-Critical Usage**: If you are using this module in a safety-critical context, refer to
:need:`doc__persistency_safety_manual` for the safety concept and the assumptions of use.

**Security Considerations**: For security aspects, refer to :need:`doc__persistency_security_manual`.

License
=======

The license of the module is defined in the
`LICENSE <https://github.com/eclipse-score/persistency/blob/main/LICENSE>`__ file of the repository.

Feedback and Contributions
==========================

Issues and suggestions are reported in the `issue tracker <https://github.com/eclipse-score/persistency/issues>`__.

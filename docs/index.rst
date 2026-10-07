..
   # *******************************************************************************
   # Copyright (c) 2024 Contributors to the Eclipse Foundation
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

.. _persistency_module_documentation:

Persistency Documentation
=========================

This documentation describes the Persistency module of S-CORE. The module implements the
Persistency feature with the Key-Value-Storage (KVS), which stores, retrieves and manages
key-value pairs persistently in JSON format on the file system. It provides a Rust and a C++
implementation.

The documentation follows the `SCORE module folder structure <https://eclipse-score.github.io/score/main/contribute/general/folder.html#module-folder-structure>`_
and the `SCORE building blocks concept <https://eclipse-score.github.io/process_description/main/general_concepts/score_building_blocks_concept.html>`_.
The feature requirements are maintained in the `SCORE platform repository <https://eclipse-score.github.io/score/main/features/index.html>`_.

.. toctree::
   :titlesonly:
   :hidden:
   :glob:

   module/index

Overview
--------

This repository provides a standardized setup for projects using **C++** or **Rust** and **Bazel** as a build system.
It integrates best practices for build, test, CI/CD and documentation.

Feature Documentation
----------------------

The Feature documentation covers the feature-level definition of the Persistency module, including architecture and safety planning artifacts.

.. toctree::
   :maxdepth: 1

   features/persistency/index

Module Documentation
---------------------

The Module documentation covers the module-level view, including architecture, safety management documents, and the user manual.

.. toctree::
   :maxdepth: 1

   module/index
   verification_report/module_verification_report

.. needtable::
   :filter: docname is not None and not docname.startswith("components/")
   :style: table
   :types: document
   :columns: title;id;safety;security;status
   :colwidths: 25,35,15,15,15
   :sort: title

Component Documentation
------------------------

The Components documentation provides detailed documentation for each individual library component, including requirements, architecture, and design decisions:

.. toctree::
   :maxdepth: 1

   components/index

.. needtable::
   :filter: docname is not None and "kvs/" in docname
   :style: table
   :types: document
   :columns: title;id;safety;security;status
   :colwidths: 25,35,15,15,15
   :sort: title

Examples
--------

Usage examples of the Rust implementation are located in ``score/kvs/rust_kvs/examples``:

- ``basic.rs``: creating a KVS instance with ``KvsBuilder`` and basic key-value operations
- ``defaults.rs``: usage of default values
- ``snapshots.rs``: snapshot count and snapshot restore
- ``custom_types.rs``: serialization and deserialization of custom types
- ``migration.rs``: migration between storage backends

The example ``basic.rs`` is executed with ``cargo run -p rust_kvs --example basic``, the other examples accordingly with their file name.
The integration of the module into a Bazel project is described in ``examples/README.md``.


.. _quick-start-building-testing:

Quick Start - Building and Testing
===================================

To build the module:

.. code-block:: bash

   bazel build --config=per-x86_64-linux -- //score/...

Building without an explicit ``--config`` (e.g. ``per-x86_64-linux``, ``per-x86_64-qnx``, ``per-arm64-qnx``) is not supported.

To run all tests:

.. code-block:: bash

   bazel test //...

To run Unit Tests:

.. code-block:: bash

   bazel test //:unit_tests

To run Component / Feature Integration Tests:

.. code-block:: bash

   bazel test //:cit_tests


Module Build Configuration
---------------------------

The ``project_config.bzl`` file at the root of the module defines metadata used by Bazel macros.
This file controls build behavior and project-specific settings. It should follow the S-CORE definition.
See `S-CORE user guide for project_config.bzl <https://eclipse-score.github.io/score/main/users_guide/building_simple_application/first_score_module.html#project-config-bzl>`_ for details.

Example:

.. code-block:: python

   PROJECT_CONFIG = {
       "asil_level": "ASIL_B",
       "source_code": ["cpp", "rust"],
   }

The configuration enables conditional build behavior:

* **Language-specific tools**: For C++ code, tools like ``clang-tidy`` are used; for Rust code, ``clippy`` is used
* **Safety level**: The ASIL level affects safety-related build settings and validation
* **Source code languages**: The build system optimizes for the configured languages

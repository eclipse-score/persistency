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

.. contents:: Table of Contents
   :depth: 2
   :local:

Module / Feature Documentation
------------------------------

.. toctree::
   :maxdepth: 1

   features/persistency/index
   module/index
   module/manuals/index
   module/release/release_note
   module/safety_mgt/index
   module/security_mgt/index
   verification_report/module_verification_report
   components/index

.. _module_documents_docs_features_persistency:

Module / Feature documentation overview
+++++++++++++++++++++++++++++++++++++++

.. needtable::
   :filter: docname is not None and not docname.startswith("components/")
   :style: table
   :types: document
   :columns: title;id;safety;security;status
   :colwidths: 25,35,15,15,15
   :sort: title


Component documentation
-----------------------

See :ref:`component_documentation` for details.


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



Quick Start
-----------

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

Module Configuration
--------------------

The ``project_config.bzl`` file in the root of the repository defines metadata used by Bazel macros.
The Persistency module uses the following configuration:

.. code-block:: python

   PROJECT_CONFIG = {
       "asil_level": "ASIL_B",
       "source_code": ["cpp", "rust"],
   }

See `S-CORE user guide for project_config.bzl <https://eclipse-score.github.io/score/main/users_guide/building_simple_application/first_score_module.html#project-config-bzl>`_ for details.

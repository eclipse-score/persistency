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

This documentation describes the structure, usage and configuration of the Bazel-based C++/Rust module template according to the `SCORE module folder structure <https://eclipse-score.github.io/score/main/contribute/general/folder.html#module-folder-structure>`_ and the `SCORE building blocks concept <https://eclipse-score.github.io/process_description/main/general_concepts/score_building_blocks_concept.html>`_.

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

Component Documentation
------------------------

The Components documentation provides detailed documentation for each individual library component, including requirements, architecture, and design decisions:

.. toctree::
   :maxdepth: 1

   components/index


Examples
--------

No examples yet.


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
       "asil_level": "QM",
       "source_code": ["cpp", "rust"]
   }

The configuration enables conditional build behavior:

* **Language-specific tools**: For C++ code, tools like ``clang-tidy`` are used; for Rust code, ``clippy`` is used
* **Safety level**: The ASIL level affects safety-related build settings and validation
* **Source code languages**: The build system optimizes for the configured languages

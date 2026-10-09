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

This user manual describes how to integrate and use the Persistency module from the perspective of an
application developer. The module provides the Key-Value-Storage (KVS), which stores key-value pairs persistently
as JSON files on the file system. The KVS is available as C++ library and as Rust crate.

For building and testing the module itself, refer to :ref:`persistency_module_documentation`.

Environment Needs
=================

* **C++**: C++17
* **Rust**: edition 2021, Ferrocene toolchain of ``score_toolchains_rust``
* **Build system**: Bazel 8.7.0 (see ``.bazelversion``)
* **Target platforms**: x86_64 Linux (GCC 12.2.0), x86_64 QNX SDP 8.0.0, aarch64 QNX SDP 8.0.0
  (Bazel configurations ``per-x86_64-linux``, ``per-x86_64-qnx``, ``per-arm64-qnx``)

Dependencies
------------

* C++ implementation: ``score_baselibs`` (filesystem, json, mw/log, result), Apache License 2.0
* Rust implementation: ``score_baselibs`` (``score_log``), Apache License 2.0;
  OSS crates ``tinyjson`` 2.5.1 (MIT) and ``adler32`` 1.2.0 (Zlib) via ``score_crates``

The complete list of dependencies and versions is defined in ``MODULE.bazel``.

Module Configuration Details
============================

A KVS instance is configured by its builder when it is opened.

.. list-table:: KVS instance configuration
   :header-rows: 1
   :widths: 20,30,30,20

   * - Setting
     - C++ (``KvsBuilder``)
     - Rust (``KvsBuilder``)
     - Default
   * - Instance ID
     - constructor ``KvsBuilder(InstanceId)``
     - ``KvsBuilder::new(InstanceId)``
     - mandatory
   * - Default values
     - ``need_defaults_flag(OpenNeedDefaults)``
     - ``defaults(KvsDefaults)``
     - ``Optional``
   * - Persisted data
     - ``need_kvs_flag(OpenNeedKvs)``
     - ``kvs_load(KvsLoad)``
     - ``Optional``
   * - Storage directory
     - ``dir(std::string)``
     - ``JsonBackendBuilder::working_dir(PathBuf)``
     - C++: ``./data_folder/``, Rust: current working directory
   * - Maximum number of snapshots
     - fixed (``KVS_MAX_SNAPSHOTS``)
     - ``JsonBackendBuilder::snapshot_max_count(usize)``
     - 3

The modes for default values and persisted data are:

* ``Required``: the data must exist, otherwise opening the instance fails.
* ``Optional``: the data is loaded if it exists, otherwise the instance starts empty.
* ``Ignored``: the data is not loaded.

Configuration Effects
---------------------

The data of an instance is stored in the storage directory in the following files:

* ``kvs_<instance id>_0.json`` and ``kvs_<instance id>_0.hash``: current data and its Adler-32 checksum
* ``kvs_<instance id>_<n>.json`` and ``kvs_<instance id>_<n>.hash``: snapshots ``1`` to the maximum number of snapshots
* ``kvs_<instance id>_default.json`` and ``kvs_<instance id>_default.hash``: default values

On opening, the checksum of the loaded files is verified. If it does not match, opening fails with
``ValidationFailed`` and the data is not used. Changed values are held in memory and written to the files on
``flush()``; each flush moves the previous data to the snapshots. In the Rust implementation, ``flush()`` is skipped
if the maximum number of snapshots is set to 0. The Rust implementation provides at most
10 instances per process; opening an instance ID again returns the already opened instance.

The default values file contains each value with its type and value (e.g. ``"timeout": {"t": "i32", "v": 30}``).
The default values file and its checksum file (Adler-32 checksum of the file content, 4 bytes big-endian) are
provided by the integrator; ``create_defaults_file`` in ``score/kvs/rust_kvs/examples/defaults.rs`` shows how both
files are created. The command line tool ``kvs_tool`` (``score/kvs/rust_kvs_tool``) reads and modifies the data of
KVS instance 0.

Examples
========

C++:

.. code-block:: cpp

   #include "score/kvs/kvsbuilder.hpp"

   using namespace score::mw::per::kvs;

   auto open_res = KvsBuilder(InstanceId(0))
                       .need_defaults_flag(OpenNeedDefaults::Optional)
                       .need_kvs_flag(OpenNeedKvs::Optional)
                       .dir("/var/data/kvs/")
                       .build();
   if (!open_res) { /* handle open_res.error() */ }
   Kvs kvs = std::move(open_res.value());

   if (!kvs.set_value("pi", KvsValue(3.14))) { /* handle error */ }
   auto value = kvs.get_value("pi");
   if (!kvs.flush()) { /* handle error */ }

Rust:

.. code-block:: rust

   use rust_kvs::prelude::*;
   use std::path::PathBuf;

   let kvs = KvsBuilder::new(InstanceId(0))
       .defaults(KvsDefaults::Optional)
       .kvs_load(KvsLoad::Optional)
       .backend(Box::new(
           JsonBackendBuilder::new().working_dir(PathBuf::from("/var/data/kvs")).build(),
       ))
       .build()?;

   kvs.set_value("number", 123.0)?;
   let number = kvs.get_value_as::<f64>("number")?;
   kvs.flush()?;

Executable Rust examples are located in ``score/kvs/rust_kvs/examples`` (basic operations, default values,
snapshots, custom types, migration between backends). They are executed with
``cargo run -p rust_kvs --example basic`` and accordingly with the other file names.

API Reference
=============

The API is documented in the source code:

* C++: ``score/kvs/kvs.hpp``, ``score/kvs/kvsbuilder.hpp``, ``score/kvs/kvsvalue.hpp``, ``score/kvs/error.hpp``
  (namespace ``score::mw::per::kvs``)
* Rust: crate ``rust_kvs`` (``score/kvs/rust_kvs``), documentation generated with ``cargo doc -p rust_kvs``

Supported value types are signed and unsigned 32 and 64 bit integers, 64 bit floating point, boolean, string,
null, array and object (``KvsValue``).

Performance Considerations
==========================

No performance measurements are documented for the module yet. The following characteristics result from the
design:

* All values of an instance are held in memory; opening an instance reads the complete data file.
* ``flush()`` writes the complete data file and its checksum and rotates the snapshot files, so its duration
  depends on the size of the stored data and on the file system.
* Read and write accesses to values do not access the file system.

Integration Guidelines
======================

Integrating with Your Project
-----------------------------

1. Add the module to your Bazel module:

   .. code-block:: python

      # In your MODULE.bazel
      bazel_dep(name = "score_persistency", version = "0.3.5")

2. Reference the library in your build files:

   .. code-block:: python

      cc_library(
          name = "my_target",
          deps = ["@score_persistency//score/kvs"],
      )

      rust_library(
          name = "my_rust_target",
          deps = ["@score_persistency//score/kvs/rust_kvs"],
      )

3. Include ``score/kvs/kvsbuilder.hpp`` (C++) or use ``rust_kvs::prelude`` (Rust).

The latest released version is 0.3.5 (`S-CORE Bazel registry <https://github.com/eclipse-score/bazel_registry/tree/main/modules/score_persistency>`__).
In version 0.3.5 the C++ library is provided as ``@score_persistency//score/kvs:kvs_cpp``. On the main branch the
target is ``@score_persistency//score/kvs``; ``kvs_cpp`` remains available as deprecated alias.

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

This module is licensed under the Apache License Version 2.0. See the ``LICENSE`` file in the repository.

Feedback and Contributions
==========================

Issues and suggestions are reported in the `issue tracker <https://github.com/eclipse-score/persistency/issues>`__.

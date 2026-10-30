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

Persistency Release Note
========================

.. document:: Persistency Release Note
   :id: doc__persistency_release_note
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__module_sw_release_note[version==1]
   :tags: persistency


| **Module Name:** Persistency
| **Release Tag:** v0.4.0
| **Origin Release Tag:** v0.3.5
| **Release Date:** TBD (set on tagging)


Overview
^^^^^^^^

This document provides an overview of the changes, improvements, and bug fixes included in the software module release version v0.4.0
as compared to the module's origin release (which is usually the previous release).

Disclaimer
----------

This release note does not "release for production", as it does not come with a safety argumentation and a performed safety assessment.
The work products compiled in the safety package are created with care according to a process satisfying standards, but the project,
being a non-profit and open source organization, can not take over any liability for its content.

Changes to the Module
^^^^^^^^^^^^^^^^^^^^^

The complete list of changes is available in the
`GitHub comparison v0.3.5...v0.4.0 <https://github.com/eclipse-score/persistency/compare/v0.3.5...v0.4.0>`_.

New Features
------------

- **Maximum storage size** configurable at compile time (`#165`_).
- **Maximum number of snapshots** configurable (C++), :need:`comp_req__kvs__snapshot_max_num_cfg` (`#383`_).
- **Load a selected snapshot** or start empty, :need:`comp_req__kvs__snapshot_restore_api` (`#464`_, `#484`_).
- **New C++ APIs** ``remove_all_keys()``, ``get_storage_file_size()``, ``discard_pending_changes()``,
  :need:`comp_req__kvs__discard_pending_changes_api` (`#421`_).
- **Key validation** with new error ``InvalidKey``, :need:`comp_req__kvs__key_length`, :need:`comp_req__kvs__key_encoding`
  (`#308`_, `#508`_, `#509`_).
- **SBOM generation** (SPDX 2.3, CycloneDX 1.6) via ``score_sbom`` (`#430`_).

Improvements
------------

- **Durable flush (Rust):** data and hash file synced to storage, :need:`comp_req__kvs__explicit_flush` (`#489`_).
- **C++ implementation:** ``vajson`` JSON library, pointer-free array/object types, reorganised library,
  migration to ``score::Result<void>`` (`#428`_, `#437`_, `#461`_, `#455`_).

Bug Fixes
---------

- ``get_value()`` returns an error for non-existing keys (C++), :need:`comp_req__kvs__value_get_api` (`#417`_).
- Duplicate spaces in KVS log messages removed (C++) (`#403`_).

Other changes by Label
----------------------

- **Documentation:** Requirements, architecture, safety and security documents updated; :need:`doc__persistency_user_manual` added.
- **Refactor / Build / CI:** see the GitHub comparison above.

Compatibility
^^^^^^^^^^^^^

- **Dependencies:** as declared in ``MODULE.bazel`` of tag v0.4.0, main dependencies ``score_baselibs`` 0.2.13,
  ``score_logging`` 0.2.4, ``score_platform`` 0.7.3, ``score_process_description`` 2.1.2, ``score_docs_as_code`` 8.4.0.
  The full dependency inventory is provided by the SBOM (``//:sbom_*``).

Performed Verification
^^^^^^^^^^^^^^^^^^^^^^

This release note is based on the verification as documented in module verification report
:need:`mod_vrep__persistency__report`.

Known Issues
------------

- **Non-atomic storage of data and hash file** (`#515`_), :need:`comp_req__kvs__atomic_store`:
  an interrupted write is detected as checksum mismatch on the next load (:need:`comp_req__kvs__pers_data_csum_vrfy`).
- **Booleans stored as 0/1 by C++** (`#495`_): the Rust-based KVS tool cannot determine the type of these values.
- **Large 64-bit integers rounded in JSON (Rust)** (`#204`_): exact storage is only given within the IEEE 754 double integer range.
- **No protection against multi-process access** (`#494`_), :need:`comp_req__kvs__single_process_access`:
  exclusive access per KVS instance has to be ensured by the user.

Known Vulnerabilities
---------------------

- No open vulnerabilities are known for this release (GitHub Dependabot alerts: none open).

Upgrade Instructions
^^^^^^^^^^^^^^^^^^^^

1. **C++ Bazel labels:** use ``//score/kvs`` instead of the deprecated ``//:kvs_cpp`` and ``//score/kvs:kvs_cpp``.
2. **C++ includes:** use full include paths (e.g. ``score/kvs/kvs.hpp``); ``includes = ["."]`` was dropped.
3. **C++ API:** replace ``score::ResultBlank`` by ``score::Result<void>`` and ``KVS_MAX_SNAPSHOTS`` by ``KvsBuilder::snapshot_max_count()``;
   handle the error of ``get_value()`` for non-existing keys.
4. **Keys (C++ and Rust):** keys must comply with :need:`comp_req__kvs__key_length` and :need:`comp_req__kvs__key_encoding`;
   storage files with invalid keys are rejected on load with ``InvalidKey``.

For any questions or support, please contact the *Project lead* or raise an issue/discussion.

.. _#165: https://github.com/eclipse-score/persistency/pull/165
.. _#204: https://github.com/eclipse-score/persistency/issues/204
.. _#308: https://github.com/eclipse-score/persistency/pull/308
.. _#383: https://github.com/eclipse-score/persistency/pull/383
.. _#403: https://github.com/eclipse-score/persistency/pull/403
.. _#417: https://github.com/eclipse-score/persistency/pull/417
.. _#421: https://github.com/eclipse-score/persistency/pull/421
.. _#428: https://github.com/eclipse-score/persistency/pull/428
.. _#430: https://github.com/eclipse-score/persistency/pull/430
.. _#437: https://github.com/eclipse-score/persistency/pull/437
.. _#455: https://github.com/eclipse-score/persistency/pull/455
.. _#461: https://github.com/eclipse-score/persistency/pull/461
.. _#464: https://github.com/eclipse-score/persistency/pull/464
.. _#484: https://github.com/eclipse-score/persistency/pull/484
.. _#489: https://github.com/eclipse-score/persistency/pull/489
.. _#494: https://github.com/eclipse-score/persistency/issues/494
.. _#495: https://github.com/eclipse-score/persistency/issues/495
.. _#508: https://github.com/eclipse-score/persistency/pull/508
.. _#509: https://github.com/eclipse-score/persistency/pull/509
.. _#515: https://github.com/eclipse-score/persistency/issues/515

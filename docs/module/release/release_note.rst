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

Persistency Release Note
========================

.. note:: Document header

.. document:: Persistency Release Note
   :id: doc__persistency_release_note
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__module_sw_release_note[version==1]
   :tags: persistency


| **Module Name:** Persistency
| **Release Tag:** v0.5.0
| **Origin Release Tag:** v0.4.1
| **Release Date:** 2026-11-20


Overview
^^^^^^^^

This document provides an overview of the changes, improvements, and bug fixes included in the software module release version v0.5.0
as compared to the module's origin release (which is usually the previous release).

This release introduces a shared C++ KVS instance registry with breaking API changes and expands build and verification coverage; see the `release comparison <https://github.com/eclipse-score/persistency/compare/v0.4.1...v0.5.0>`_.

Disclaimer
----------

This release note does not "release for production", as it does not come with a safety argumentation and a performed safety assessment.
The work products compiled in the safety package are created with care according to a process satisfying standards, but the project,
being a non-profit and open source organization, can not take over any liability for its content.

Changes to the Module
^^^^^^^^^^^^^^^^^^^^^

The release description contains the generated changelog, including the complete list of merged pull requests and dependency updates:
`GitHub releases <https://github.com/eclipse-score/persistency/releases>`_.
This note focuses on behavior and compatibility changes relevant to module users.

New Features
------------

- **C++ instance registry:** ``KvsBuilder::Build()`` returns a shared pointer to a process-local KVS instance. Repeated builds with the same instance ID and identical options share the same object, including unflushed changes, without reloading files.
- **C++ instance lifecycle:** ``score::kvs::Unregister()`` removes an instance once all caller-held shared pointers have been released. The registry otherwise retains the instance until process exit.

Improvements
------------

- **C++ thread safety:** snapshot counting, file-path queries and storage-file size queries now acquire the instance mutex.
- **Developer tooling:** improved IDE support includes compile-command extraction, VS Code tasks and Rust formatting configuration.

Bug Fixes
---------

- **C++ duplicate instances:** the registry prevents independently opened objects for the same instance ID within one process. Opening an already registered ID with different options fails with ``kInstanceParametersMismatch``.

Other changes by Label
----------------------

- **Verification coverage:** added Linux aarch64 tests under QEMU user emulation, QNX x86_64 and aarch64 unit tests in QEMU VMs, and Linux AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer checks.
- **Integration testing:** expanded C++ and Rust tests for explicit flush and persistence across restarts.
- **Build/CI maintenance:** release checks reuse the build, Cargo, lint, sanitizer, coverage and documentation workflows; coverage tooling moved to ``tools/coverage`` with a scope check.
- **Dependencies and documentation:** updated module and CI dependencies and integrated Quick Start into the module documentation; see the generated changelog for the complete list.

Compatibility
^^^^^^^^^^^^^

- **C++ source and ABI compatibility:** this release is not a drop-in replacement for v0.4.1. The builder result type and instance ownership model changed, ``Kvs::Open()`` and its deprecated ``open()`` alias were removed, and ``Kvs`` is no longer movable. Rebuild consumers after adapting them as described below.
- **C++ instance IDs:** valid IDs are now limited to 0 through 9 (``kMaxInstanceCount`` is 10); other IDs fail with ``kInvalidInstanceId``.
- **Rust API and stored data:** no Rust public API or persisted JSON/hash format changes were introduced.
- **Dependencies:** as declared in ``MODULE.bazel`` of tag v0.5.0
  The full dependency inventory is provided by the SBOM (``//:sbom_*``).

Performed Verification
^^^^^^^^^^^^^^^^^^^^^^

This release note is based on the verification as documented in module verification report
:need:`mod_vrep__persistency__report`.

Known Issues
------------

- **Non-atomic storage of data and hash file** (`#515`_), :need:`comp_req__kvs__atomic_store`:
  an interrupted write is detected as checksum mismatch on the next load (:need:`comp_req__kvs__pers_data_csum_vrfy`).
- **Large 64-bit integers rounded in JSON (Rust)** (`#204`_): exact storage is only given within the IEEE 754 double integer range.
- **No protection against multi-process access** (`#494`_), :need:`comp_req__kvs__single_process_access`: the new registry coordinates instances within one process only; exclusive access across processes still has to be ensured by the user.

Known Vulnerabilities
---------------------

- No open vulnerabilities are known for this release (GitHub Dependabot alerts: none open).

Upgrade Instructions
^^^^^^^^^^^^^^^^^^^^

1. **Open through the builder:** replace ``Kvs::Open()`` and ``Kvs::open()`` with ``KvsBuilder`` calls using equivalent options and ``Build()``.
2. **Use shared ownership:** ``Build()`` and its deprecated ``build()`` alias now return ``score::Result<std::shared_ptr<Kvs>>`` instead of ``score::Result<Kvs>``. Check the result, then access the store through the returned shared pointer rather than moving a ``Kvs`` value.
3. **Coordinate instance configuration:** use IDs below ``kMaxInstanceCount`` and identical builder options for all users of an ID. Different directories do not create independent instances under the same ID; handle ``kInvalidInstanceId`` and ``kInstanceParametersMismatch``.
4. **Account for shared state:** mutations, including pending changes, resets and snapshot restores, affect every holder of the instance. Building again does not reload data or change its configuration.
5. **Manage reopening explicitly:** flush changes that must persist, release all caller-held shared pointers, then call ``score::kvs::Unregister()`` before rebuilding to reload files or change options. Unregistering does not flush; it fails with ``kResourceBusy`` while callers still hold pointers, or ``kInvalidInstanceId`` if the ID is invalid or not registered.
6. **Rebuild C++ consumers:** adapt value-based storage and ownership assumptions to shared pointers. No stored-data migration is required.

For any questions or support, please contact the *Project lead* or raise an issue/discussion.

.. _#204: https://github.com/eclipse-score/persistency/issues/204
.. _#494: https://github.com/eclipse-score/persistency/issues/494
.. _#515: https://github.com/eclipse-score/persistency/issues/515

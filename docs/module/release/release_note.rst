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
| **Release Tag:** v0.4.1
| **Origin Release Tag:** v0.4.0
| **Release Date:** 2026-11-10


Overview
^^^^^^^^

This document provides an overview of the changes, improvements, and bug fixes included in the software module release version v0.4.1
as compared to the module's origin release (which is usually the previous release).

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

No new KVS operations or storage capabilities were introduced in this maintenance release.

Improvements
------------

- **Instance locking:** operations on the same KVS instance now wait for the lock instead of failing immediately;
  the C++ ``KMutexLockFailed`` error was removed.
- **C++ filesystem access:** the implementation now uses ``score::filesystem`` instead of ``std::filesystem``.
- **Logging:** KVS instances share a logger and emit more informative messages.

Bug Fixes
---------

No additional user-visible bug fixes are called out separately from the generated changelog.

Other changes by Label
----------------------

- **Integration testing:** C++ and Rust component integration tests now run through ITF on Linux (Docker) and QNX x86_64 (QEMU).
- **Documentation and build/CI maintenance:** see the generated changelog in the GitHub release description.

Compatibility
^^^^^^^^^^^^^

- **Dependencies:** as declared in ``MODULE.bazel`` of tag v0.4.1
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
- **No protection against multi-process access** (`#494`_), :need:`comp_req__kvs__single_process_access`:
  exclusive access per KVS instance has to be ensured by the user.

Known Vulnerabilities
---------------------

- No open vulnerabilities are known for this release (GitHub Dependabot alerts: none open).

Upgrade Instructions
^^^^^^^^^^^^^^^^^^^^

1. **C++ API:** remove handling for ``KMutexLockFailed``; contending calls now wait for the instance lock.
2. **C++ API:** assignments to rvalue ``Kvs`` or ``KvsValue`` objects are no longer supported.
3. **C++ API:** ``KvsValue::GetValue()`` on a temporary now returns by value.
4. **C++ ABI:** ``KvsValue::Type`` now has underlying type ``std::uint8_t``; rebuild C++ consumers.

For any questions or support, please contact the *Project lead* or raise an issue/discussion.

.. _#204: https://github.com/eclipse-score/persistency/issues/204
.. _#494: https://github.com/eclipse-score/persistency/issues/494
.. _#515: https://github.com/eclipse-score/persistency/issues/515

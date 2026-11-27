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
| **Release Tag:** v0.6.0
| **Origin Release Tag:** v0.5.0
| **Release Date:** 2026-11-27

Overview
^^^^^^^^

Compared with v0.5.0, this release adds additional numeric types, hardens C++ registry usage and updates QNX build prerequisites. Numeric compatibility changes require a minor release.

Disclaimer
----------

This release note does not "release for production", as it does not come with a safety argumentation and a performed safety assessment. The work products compiled in the safety package are created with care according to a process satisfying standards, but the project, being a non-profit and open source organization, can not take over any liability for its content.

Changes to the Module
^^^^^^^^^^^^^^^^^^^^^

For the complete changelog and comparison, see the `v0.6.0 release page <https://github.com/eclipse-score/persistency/releases/tag/v0.6.0>`_.

New Features
------------

- **Numeric types:** C++ and Rust ``KvsValue`` support native ``i8``, ``i16``, ``u8``, ``u16`` and ``f32`` values and JSON tags.

Improvements
------------

- **Numeric handling:** stricter JSON numeric validation and round-trip support for finite ``f32`` values.
- **C++ diagnostics:** ``Build()``, its deprecated ``build()`` alias and ``Unregister()`` are now ``[[nodiscard]]``.

Bug Fixes
---------

- **C++ registry bypass:** made the ``Kvs`` constructor private; stores must be opened through ``KvsBuilder``.
- **Safety documentation:** restored KVS assumptions of use in the safety manual and corrected safety-plan tailoring.

Other changes by Label
----------------------

- **Verification:** expanded integrity, key-value, recovery, flush-failure and numeric tests; aligned sanitizer configurations and fixed shadowing warnings.
- **Build:** updated dependencies, including ``score_bazel_cpp_toolchains`` v1.1.0 and QNX SDP 8.0.5.
- **Documentation:** marked module maturity experimental and corrected requirement statistics.

Compatibility
^^^^^^^^^^^^^

- **C++:** added variant alternatives change ABI, variant indices and most ``KvsValue::Type`` numeric values. Narrow arguments now select native types instead of wider types.
- **Rust:** exhaustive ``KvsValue`` matches need the new variants. Typed reads of ``i8``/``i16``, ``u8``/``u16`` and ``f32`` require native tags; previously stored ``i32``/``u32``/``f64`` values need explicit conversion or fail with ``DeserializationFailed``.
- **Storage:** existing tags remain readable, subject to stricter validation; JSON envelopes and hashes are unchanged. Older releases cannot read the new tags.
- **QNX:** default SDP 8.0.5 packages additionally require authenticated GitHub downloads; a valid QNX license remains required.
- **Dependencies:** see ``MODULE.bazel`` at v0.6.0 and the SBOM (``//:sbom_*``).

Performed Verification
^^^^^^^^^^^^^^^^^^^^^^

Verification is documented in :need:`mod_vrep__persistency__report`. QNX boundary-test exclusions remain as noted below.

Known Issues
------------

- **Non-atomic data/hash writes** (`eclipse-score/persistency#515`_): interrupted writes can cause checksum mismatches (:need:`comp_req__kvs__atomic_store`, :need:`comp_req__kvs__pers_data_csum_vrfy`).
- **Rust 64-bit integers** (`eclipse-score/persistency#204`_): JSON can round integers outside the exact IEEE 754 double range.
- **Multi-process access** (`eclipse-score/persistency#494`_): users must ensure exclusive access (:need:`comp_req__kvs__single_process_access`).
- **QNX double parsing** (`eclipse-score/baselibs#894 <https://github.com/eclipse-score/baselibs/issues/894>`_): boundary values can fail to parse or lose precision. Affected checks remain excluded on QNX, not Linux; the SDP update is not a confirmed fix.
- **Signed zero:** C++/Rust behavior differs; see :need:`doc__persistency_user_manual`.

Known Vulnerabilities
---------------------

- No open Dependabot alerts as of 2026-11-27; this is not a security assessment.

Upgrade Instructions
^^^^^^^^^^^^^^^^^^^^

1. Back up stores, defaults and snapshots. Keep wider tags while rollback to older readers is required.
2. Rebuild C++ consumers; adapt visitors, Rust matches and index assumptions. Explicitly widen C++ narrow inputs to retain previous tags.
3. Migrate Rust narrow-type reads by reading the original wider type, range-checking/converting and rewriting; include defaults and snapshots. Migration is not automatic.
4. Construct C++ stores through ``KvsBuilder`` and check registry results. Flush before unregistering (:need:`aou_req__kvs__flush_before_unregister`); otherwise pending changes are lost.
5. Configure QNX package authentication: `QNX SDK access <https://github.com/eclipse-score/persistency/blob/main/README.md#qnx-sdk-access>`_.

For any questions or support, please contact the *Project lead* or raise an issue/discussion.

.. _eclipse-score/persistency#204: https://github.com/eclipse-score/persistency/issues/204
.. _eclipse-score/persistency#494: https://github.com/eclipse-score/persistency/issues/494
.. _eclipse-score/persistency#515: https://github.com/eclipse-score/persistency/issues/515

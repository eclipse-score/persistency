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

KVS Assumptions of Use
======================

.. document:: KVS AoU
   :id: doc__kvs_aou
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_comp_aou[version==1]
   :tags: Persistency KVS

Assumptions of use resulting from :need:`doc__kvs_fmea` and :need:`doc__kvs_dfa`.

Component AoU
-------------

.. aou_req:: File System
   :id: aou_req__kvs__filesystem
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :tags: Persistency KVS, environment

   The user shall ensure that the file system and storage used for the KVS directory provide deterministic
   operations with a bounded execution time (open, read, write, ``fdatasync``, ``rename``, directory creation and
   file queries) for the maximum storage size and the maximum number of snapshots configured for the KVS instance,
   that they persist written data once ``fsync``/``fdatasync`` has returned, and keep renamed files consistent
   across a reset, so that data reported as flushed survives a reset.

.. aou_req:: KVS Limits Configuration
   :id: aou_req__kvs__limits
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :tags: Persistency KVS, environment

   The user shall configure the maximum storage size and the maximum number of snapshots of each KVS instance, so
   that the execution time of the KVS operations is bounded.

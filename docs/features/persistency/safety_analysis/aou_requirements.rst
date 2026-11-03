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

Persistency Assumptions of Use
==============================

.. document:: Persistency Feature AoU
   :id: doc__persistency_feat_aou
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__requirements_feat_aou[version==1]
   :tags: persistency

Assumptions of use of the feature, resulting from :need:`doc__persistency_fmea` and :need:`doc__persistency_dfa`.

AoU
---

.. aou_req:: Persistency Error handling
   :id: aou_req__persistency__error_handling
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 2
   :tags: persistency, environment

   The user shall detect and handle the unavailability of the feature persistency.
   Unavailability covers errors reported by the persistency API.

.. aou_req:: Separate KVS Instances per Software Element
   :id: aou_req__persistency__instance_separation
   :reqtype: Process
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :tags: persistency

   The user shall ensure that each FFI element (process) uses KVS storage, identified by directory and
   instance ID, which is not used by any other FFI element, so that persisted data of one FFI element cannot be
   read or modified by another FFI element.

   Note: The KVS does not synchronise access between processes. Processes which open the same directory and
   instance ID access the same files.

.. aou_req:: Start without Persisted Data
   :id: aou_req__persistency__start_without_data
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :tags: persistency

   The user shall configure, for each KVS instance, whether existing persisted data is required when opening it,
   or handle that the KVS instance provides only default values because no persisted data is available.

.. aou_req:: Flush of Safety Relevant Changes
   :id: aou_req__persistency__flush
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 1
   :tags: persistency

   The user shall flush changes which shall be persisted and shall consider changes as persisted only
   after the flush has returned without error. If the flush reports an error, the user shall treat the changes
   as not persisted.

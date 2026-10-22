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
   :realizes: wp__requirements_feat_aou
   :tags: persistency


AoU
---

.. aou_req:: Persistency Error handling
   :id: aou_req__persistency__error_handling
   :reqtype: Functional
   :security: NO
   :safety: ASIL_B
   :status: valid
   :version: 2
   :tags: persistency

   The application shall detect and handle the unavailability of the feature persistency.
   Unavailability covers errors reported by the persistency API as well as persistency calls which do
   not return or return too late (e.g. caused by blocked or delayed execution of the calling context).

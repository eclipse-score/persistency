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


KVS DFA (Dependent Failure Analysis)
====================================

.. document:: KVS DFA
   :id: doc__kvs_dfa
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__sw_component_dfa[version==1]
   :tags: Persistency KVS

The KVS component has no sub-components (:need:`doc__kvs_component_architecture`). According to
:need:`doc_concept__safety_analysis`, the results of the DFA are therefore the same as on feature level,
see :need:`doc__persistency_dfa`. No additional component level analysis is needed.

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

Component Architecture KVS
==========================

.. document:: KVS Component Architecture
   :id: doc__kvs_component_architecture
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: YES
   :realizes: wp__component_arch[version==1]

Overview
--------

.. comp:: persistency::kvs
   :id: comp__persistency_kvs
   :security: YES
   :safety: ASIL_B
   :status: valid
   :version: 1
   :implements: logic_arc_int__persistency__interface[version==1]
   :uses: logic_arc_int__baselibs__json[version==1],logic_arc_int__baselibs__filesystem[version==1],logic_arc_int__log_cpp__logging[version==1]
   :belongs_to: feat__persistency[version==1]

Static Architecture
-------------------

.. comp_arc_sta:: KVS Static View
   :id: comp_arc_sta__persistency_kvs__sv
   :status: valid
   :version: 1
   :safety: ASIL_B
   :security: NO
   :uses: logic_arc_int__baselibs__json[version==1],logic_arc_int__baselibs__filesystem[version==1],logic_arc_int__log_cpp__logging[version==1]
   :belongs_to: comp__persistency_kvs[version==1]

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

tbd

Interfaces
----------

tbd

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

.. _feature_architecture_persistency:

Architecture
============

.. document:: Persistency KVS Feature Architecture
   :id: doc__persistency_architecture
   :status: valid
   :safety: ASIL_B
   :security: NO
   :realizes: wp__feature_arch
   :tags: persistency

Overview
--------

The Key-Value-Storage (kvs) provides the capability to efficiently store,
retrieve, and manage key-value pairs in a persistent storage system.

Description
-----------

- kvs organize data as pairs, where each unique key is associated with a specific value.
  The key acts as a unique identifier for getting the value.
- The data is persisted in JSON format to the file system, providing a human-readable,
  and widely supported way to store and manage key-value pairs.
- The JSON data persisted is according to RFC-8259.

Rationale Behind Architecture Decomposition
*******************************************

- The architecture is decomposed to include a dedicated JSON parser component (json) to facilitate the persistent storage of data in JSON format.
- The architecture is decomposed to include a FileStorage component (fs) to read and write to the file system.


Glossary
--------

- User: Program code that is written by a person that initiates the given
  functionality call or receives a callback.


Static Architecture
-------------------

.. feat_arc_sta:: Static Architecture
   :id: feat_arc_sta__persistency__staticx
   :security: YES
   :safety: ASIL_B
   :includes: logic_arc_int__persistency__interface
   :fulfils: feat_req__persistency__default_value_get,feat_req__persistency__default_values,feat_req__persistency__async_completion,feat_req__persistency__integrity_check,feat_req__persistency__store_data,feat_req__persistency__load_data,feat_req__persistency__snapshot_create,feat_req__persistency__support_datatype_keys,feat_req__persistency__support_datatype_value,feat_req__persistency__variant_management,feat_req__persistency__default_value_file,feat_req__persistency__cfg,feat_req__persistency__async_api,feat_req__persistency__access_control,feat_req__persistency__concurrency
   :status: valid

   .. uml:: _assets/kvs_static_view.puml



Logical Interfaces
------------------

.. logic_arc_int:: Ikvs
   :id: logic_arc_int__persistency__interfacex
   :security: YES
   :safety: ASIL_B
   :fulfils: feat_req__persistency__async_api
   :status: valid

   .. uml:: _assets/kvs_interface.puml

.. needextend:: docname is not None and "persistency/kvs/architecture" in docname
   :+tags: persistency

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

Dynamic Architecture
--------------------

.. feat_arc_dyn:: Check if key contains default value
   :id: feat_arc_dyn__persistency__check_key_default
   :security: YES
   :safety: ASIL_B
   :fulfils: feat_req__persistency__default_values,feat_req__persistency__default_value_get
   :status: valid

   .. uml:: _assets/kvs_dyn_check_value_default.puml

.. feat_arc_dyn:: Delete key from KVS instance
   :id: feat_arc_dyn__persistency__delete_key
   :security: YES
   :safety: ASIL_B
   :fulfils: feat_req__persistency__support_datatype_keys,feat_req__persistency__support_datatype_value
   :status: valid

   .. uml:: _assets/kvs_dyn_delete_data_key.puml

.. feat_arc_dyn:: Flush to permanent storage
   :id: feat_arc_dyn__persistency__flush
   :security: YES
   :safety: ASIL_B
   :fulfils: feat_req__persistency__store_data,feat_req__persistency__snapshot_create,feat_req__persistency__integrity_check,feat_req__persistency__snapshot_restore
   :status: valid

   .. uml:: _assets/kvs_dyn_flush_local_repr_to_file.puml

.. feat_arc_dyn:: Read key value
   :id: feat_arc_dyn__persistency__read_key
   :security: YES
   :safety: ASIL_B
   :fulfils: feat_req__persistency__support_datatype_keys,feat_req__persistency__support_datatype_value,feat_req__persistency__default_values,feat_req__persistency__default_value_get
   :status: valid

   .. uml:: _assets/kvs_dyn_read_data_key.puml

.. feat_arc_dyn:: Read data from permanent storage
   :id: feat_arc_dyn__persistency__read_from_storage
   :security: YES
   :safety: ASIL_B
   :fulfils: feat_req__persistency__load_data,feat_req__persistency__integrity_check,feat_req__persistency__snapshot_restore
   :status: valid

   .. uml:: _assets/kvs_dyn_read_file_into_local_repr.puml

.. feat_arc_dyn:: Write value to key
   :id: feat_arc_dyn__persistency__write_key
   :security: YES
   :safety: ASIL_B
   :fulfils: feat_req__persistency__support_datatype_keys,feat_req__persistency__support_datatype_value
   :status: valid

   .. uml:: _assets/kvs_dyn_write_data_key.puml

.. feat_arc_dyn:: Restore snapshot
   :id: feat_arc_dyn__persistency__snapshot_restore
   :security: YES
   :safety: ASIL_B
   :fulfils: feat_req__persistency__snapshot_restore,feat_req__persistency__store_data
   :status: valid

   .. uml:: _assets/kvs_dyn_restore_snapshot.puml

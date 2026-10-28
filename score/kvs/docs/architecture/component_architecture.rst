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
   :fulfils: comp_req__kvs__language_bindings[version==1],comp_req__kvs__os_abstraction[version==1],comp_req__kvs__no_dynamic_memory[version==1],comp_req__kvs__multi_instance[version==2],comp_req__kvs__single_process_access[version==1],comp_req__kvs__permission_control[version==1],comp_req__kvs__permission_err_hndl[version==1],comp_req__kvs__storage_file_permissions[version==1],comp_req__kvs__default_storage_permissions[version==1],comp_req__kvs__constraints[version==1],comp_req__kvs__constraints_priority[version==1],comp_req__kvs__snapshot_max_num_cfg[version==1],comp_req__kvs__key_naming[version==1],comp_req__kvs__key_encoding[version==1],comp_req__kvs__key_uniqueness[version==2],comp_req__kvs__key_value_overwrite[version==1],comp_req__kvs__key_length[version==1],comp_req__kvs__key_exists_api[version==1],comp_req__kvs__key_list_api[version==1],comp_req__kvs__key_remove_api[version==1],comp_req__kvs__value_data_types[version==2],comp_req__kvs__value_get_api[version==1],comp_req__kvs__value_set_api[version==1],comp_req__kvs__value_serialize[version==1],comp_req__kvs__value_default[version==2],comp_req__kvs__value_reset[version==2],comp_req__kvs__default_value_types[version==2],comp_req__kvs__default_value_query[version==2],comp_req__kvs__default_value_is_default_api[version==1],comp_req__kvs__default_value_cfg[version==2],comp_req__kvs__default_val_chksum[version==1],comp_req__kvs__reset_resistant_write[version==1],comp_req__kvs__corruption_detect_open[version==1],comp_req__kvs__open_need_kvs_behavior[version==1],comp_req__kvs__recovery_snapshot_restore[version==1],comp_req__kvs__atomic_store[version==1],comp_req__kvs__explicit_flush[version==1],comp_req__kvs__write_amplification[version==1],comp_req__kvs__cached_access[version==1],comp_req__kvs__store_reset_api[version==1],comp_req__kvs__discard_pending_changes_api[version==1],comp_req__kvs__pers_data_csum[version==1],comp_req__kvs__pers_data_csum_vrfy[version==2],comp_req__kvs__pers_data_store_bnd[version==1],comp_req__kvs__pers_data_store_fmt[version==1],comp_req__kvs__storage_backend_select[version==1],comp_req__kvs__async_api[version==2],comp_req__kvs__callback_support[version==1],comp_req__kvs__snapshot_create_api[version==1],comp_req__kvs__snapshot_create[version==1],comp_req__kvs__snapshot_overwrite[version==1],comp_req__kvs__explicit_snapshot_operations[version==1],comp_req__kvs__snapshot_id_api[version==2],comp_req__kvs__snapshot_slot_index_vali[version==1],comp_req__kvs__snapshot_source[version==1],comp_req__kvs__snapshot_slot_free_api[version==2],comp_req__kvs__snapshot_restore_api[version==1],comp_req__kvs__snapshot_remove_api[version==1],comp_req__kvs__concurrency[version==1],comp_req__kvs__pers_data_version[version==2],comp_req__kvs__pers_data_schema[version==2],comp_req__kvs__pers_data_migration[version==1],comp_req__kvs__random_access_complexity[version==1],comp_req__kvs__default_value_cfg_tool[version==1],comp_req__kvs__storage_tool[version==1]
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

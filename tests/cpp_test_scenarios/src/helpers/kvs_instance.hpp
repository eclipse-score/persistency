/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#pragma once
#include "kvs_parameters.hpp"
#include <kvsbuilder.hpp>

static score::mw::per::kvs::Kvs kvs_instance(const KvsParameters &params) {
    using namespace score::mw::per::kvs;
    InstanceId instance_id{params.instance_id};
    KvsBuilder builder{instance_id};
    if (params.need_defaults.has_value()) {
        builder = builder.need_defaults_flag(*params.need_defaults);
    }
    if (params.need_kvs.has_value()) {
        builder = builder.need_kvs_flag(*params.need_kvs);
    }
    if (params.dir.has_value()) {
        builder = builder.dir(std::string(*params.dir));
    }
    auto kvs_ptr = builder.build();
    if (!kvs_ptr) {
        throw ScenarioError(
            score::mw::per::kvs::ErrorCode::JsonParserError,
            "KVS creation failed: build() returned null (possible "
            "file not found, JSON parse error, or corruption)");
    }
    return std::move(*kvs_ptr);
}
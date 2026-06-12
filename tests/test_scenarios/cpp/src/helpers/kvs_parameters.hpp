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

#include <kvs.hpp>

struct KvsParameters
{
    /// Parse `KvsParameters` from JSON string.
    /// JSON is expected to contain `kvs_parameters` field.
    static KvsParameters from_json(const std::string& json_str);

    /// Parse `KvsParameters` from `Object`.
    /// `Object` is expected to contain `kvs_parameters` field.
    static KvsParameters from_object(const score::json::Object& object);

    score::mw::per::kvs::InstanceId instance_id;
    std::optional<bool> need_defaults;
    std::optional<bool> need_kvs;
    std::optional<std::string> dir;
    std::optional<size_t> snapshot_max_count;
};

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
#ifndef SCORE_KVS_TESTS_TEST_SCENARIOS_HELPERS_KVS_PARAMETERS_H
#define SCORE_KVS_TESTS_TEST_SCENARIOS_HELPERS_KVS_PARAMETERS_H

#include "score/json/json_parser.h"
#include "score/kvs/kvs.h"
#include <cstddef>
#include <optional>
#include <string>

struct KvsParameters
{
    /// @brief Parses `KvsParameters` from a JSON string.
    ///
    /// JSON is expected to contain `kvs_parameters` field.
    static KvsParameters FromJson(const std::string& json_str);

    /// @brief Parses `KvsParameters` from an `Object`.
    ///
    /// `Object` is expected to contain `kvs_parameters` field.
    static KvsParameters FromObject(const score::json::Object& object);

    score::kvs::InstanceId instance_id;
    std::optional<score::kvs::OpenNeedDefaults> need_defaults;
    std::optional<score::kvs::OpenNeedKvs> need_kvs;
    std::optional<std::string> dir;
    std::optional<size_t> snapshot_max_count;
};

#endif  // SCORE_KVS_TESTS_TEST_SCENARIOS_HELPERS_KVS_PARAMETERS_H

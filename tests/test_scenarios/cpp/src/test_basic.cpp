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
#include "test_basic.hpp"

#include <kvs.hpp>
#include <kvsbuilder.hpp>
#include <cassert>
#include <iostream>
#include <unordered_map>

#include "helpers/kvs_parameters.hpp"
#include "score/json/json_parser.h"
#include "score/json/json_writer.h"
#include "score/result/result.h"
#include <tracing.hpp>

static const std::string kTargetName{"cpp_test_scenarios::basic::basic"};

std::string BasicScenario::name() const
{
    return "basic";
}

void BasicScenario::run(const std::string& input) const
{
    using namespace score::mw::per::kvs;

    // Print and parse parameters.
    std::cerr << input << std::endl;

    auto params{KvsParameters::from_json(input)};

    // Set builder parameters.
    InstanceId instance_id{params.instance_id};
    KvsBuilder builder{instance_id};
    if (params.need_defaults.has_value())
    {
        builder = builder.need_defaults_flag(*params.need_defaults);
    }
    if (params.need_kvs.has_value())
    {
        builder = builder.need_kvs_flag(*params.need_kvs);
    }
    // TODO: handle dir?

    // Create KVS.
    Kvs kvs{*builder.build()};

    // Simple set/get.
    std::string key{"example_key"};
    std::string value{"example_value"};
    auto set_value_result{kvs.set_value(key, KvsValue{value})};
    if (!set_value_result)
    {
        throw std::runtime_error("Failed to set value");
    }

    auto get_value_result{kvs.get_value(key)};
    if (!get_value_result)
    {
        throw std::runtime_error{"Failed to get value"};
    }
    auto stored_kvs_value{get_value_result.value()};
    if (stored_kvs_value.getType() != KvsValue::Type::String)
    {
        throw std::runtime_error{"Invalid value type"};
    }

    auto stored_value{std::get<std::string>(stored_kvs_value.getValue())};
    if (stored_value.compare(value) != 0)
    {
        throw std::runtime_error("Value mismatch");
    }

    // Trace.
    TRACING_INFO(kTargetName, std::pair{std::string{"example_key"}, stored_value});
}

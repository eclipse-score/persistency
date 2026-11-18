/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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

#include "cit/key_value.h"

#include "helpers/kvs_instance.h"
#include "helpers/kvs_parameters.h"
#include <tracing.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

using namespace score::kvs;
using namespace score::json;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::cit::key_value"};

/// @brief "Ok(true)" or "Ok(false)" on success, "Err(<error message>)" on failure.
std::string ResultString(const score::Result<bool>& result)
{
    if (!result)
    {
        return "Err(" + std::string{result.error().Message()} + ")";
    }
    return result.value() ? "Ok(true)" : "Ok(false)";
}

/// @brief "Ok(<number>)" for an I32 value on success, "Err(<error message>)" on failure.
///
/// Throws if the value is not an I32.
std::string ResultString(const score::Result<KvsValue>& result)
{
    if (!result)
    {
        return "Err(" + std::string{result.error().Message()} + ")";
    }
    const auto* const number{std::get_if<int32_t>(&result.value().GetValue())};
    if (number == nullptr)
    {
        throw std::runtime_error{"Value is not an I32"};
    }
    return "Ok(" + std::to_string(*number) + ")";
}

/// @brief Runs one operation of the input on the KVS: ["set", key, I32 value], ["remove", key] or ["reset"].
score::Result<void> Apply(Kvs& kvs, const List& operation)
{
    const std::string name{operation.at(0).As<std::string>().value().get()};
    if (name == "set")
    {
        const std::string key{operation.at(1).As<std::string>().value().get()};
        return kvs.SetValue(key, KvsValue{operation.at(2).As<int32_t>().value()});
    }
    if (name == "remove")
    {
        const std::string key{operation.at(1).As<std::string>().value().get()};
        return kvs.RemoveKey(key);
    }
    if (name == "reset")
    {
        return kvs.Reset();
    }
    throw std::runtime_error{"Unknown operation: " + name};
}
}  // namespace

/// @brief Opens the KVS and runs the operations of the input in memory, without a flush.
/// Then logs every written key under "listed_key", and for each key of the input the results of
/// KeyExists, IsValueDefault and GetValue.
class QueryAfterOperations final : public Scenario
{
  public:
    ~QueryAfterOperations() final = default;

    std::string name() const final
    {
        return "query_after_operations";
    }

    void run(const std::string& input) const final
    {
        const JsonParser parser;
        const auto any_result{parser.FromBuffer(input)};
        if (!any_result)
        {
            throw score::result::Error(any_result.error());
        }
        const auto& object{any_result.value().As<Object>().value().get()};
        const auto& operations{object.at("operations").As<List>().value().get()};
        const auto& keys{object.at("keys").As<List>().value().get()};

        auto kvs{KvsInstance(KvsParameters::FromValue(object))};

        for (const auto& operation : operations)
        {
            const auto result{Apply(kvs, operation.As<List>().value().get())};
            if (!result)
            {
                throw std::runtime_error{"Operation failed: " + std::string{result.error().Message()}};
            }
        }

        const auto listed_keys{kvs.GetAllKeys()};
        if (!listed_keys)
        {
            throw std::runtime_error{"Failed to get keys: " + std::string{listed_keys.error().Message()}};
        }
        for (const auto& listed_key : listed_keys.value())
        {
            TRACING_INFO(kTargetName, std::pair{std::string{"listed_key"}, listed_key});
        }

        for (const auto& key_value : keys)
        {
            const std::string key{key_value.As<std::string>().value().get()};
            TRACING_INFO(kTargetName,
                         std::pair{std::string{"key"}, key},
                         std::pair{std::string{"key_exists"}, ResultString(kvs.KeyExists(key))},
                         std::pair{std::string{"value_is_default"}, ResultString(kvs.IsValueDefault(key))},
                         std::pair{std::string{"value"}, ResultString(kvs.GetValue(key))});
        }
    }
};

ScenarioGroup::Ptr KeyValueGroup()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"key_value", {std::make_shared<QueryAfterOperations>()}, {}}};
}

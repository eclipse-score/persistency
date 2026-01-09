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

#include "supported_datatypes.hpp"

#include "helpers/kvs_instance.hpp"
#include "helpers/kvs_parameters.hpp"
#include "tracing.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace score::mw::per::kvs;
using namespace score::json;

// Helper to convert KvsValue to string for logging
std::string kvs_value_to_string(const KvsValue &v)
{
    switch (v.getType())
    {
    case KvsValue::Type::i32:
        return std::to_string(std::get<int32_t>(v.getValue()));
    case KvsValue::Type::u32:
        return std::to_string(std::get<uint32_t>(v.getValue()));
    case KvsValue::Type::i64:
        return std::to_string(std::get<int64_t>(v.getValue()));
    case KvsValue::Type::u64:
        return std::to_string(std::get<uint64_t>(v.getValue()));
    case KvsValue::Type::f64:
        return std::to_string(std::get<double>(v.getValue()));
    case KvsValue::Type::Boolean:
        return std::get<bool>(v.getValue()) ? "true" : "false";
    case KvsValue::Type::String:
        return std::get<std::string>(v.getValue());
    case KvsValue::Type::Null:
        return "null";
    case KvsValue::Type::Array:
        return "[array]";
    case KvsValue::Type::Object:
        return "{object}";
    default:
        return "unknown";
    }
}

namespace
{
    const std::string kTargetName{"cpp_test_scenarios::supported_datatypes"};

    void info_log(const std::string &keyname)
    {
        TRACING_INFO(kTargetName,
                     std::make_pair(std::string("key"), std::string(keyname)));
    }
    // Overloaded info_log to print name and value
    void info_log(const std::string &name, const std::string &value)
    {
        TRACING_INFO(kTargetName, std::make_pair(name, value));
    }
} // namespace

class SupportedDatatypesKeys : public Scenario
{

public:
    ~SupportedDatatypesKeys() final = default;

    std::string name() const final { return "keys"; }

    void run(const std::string &input) const final
    {
        // Create KVS instance with provided params.
        KvsParameters params{KvsParameters::from_json(input)};
        Kvs kvs = kvs_instance(params);

        std::vector<std::string> keys_to_check = {
            "example",
            u8"emoji ✅❗😀", // UTF-8 encoded string literal
            u8"greek ημα"     // UTF-8 encoded string literal
        };
        for (const auto &s : keys_to_check)
        {
            kvs.set_value(s, KvsValue(nullptr));
        }

        auto keys_in_kvs = kvs.get_all_keys();
        if (keys_in_kvs.has_value())
        {
            for (const auto &s : keys_in_kvs.value())
            {
                info_log(s);
            }
        }
        else
        {
            throw keys_in_kvs.error();
        }
    }
};

class SupportedDatatypesValues : public Scenario
{

    KvsValue value;

public:
    explicit SupportedDatatypesValues(const KvsValue &v) : value(v) {}

    ~SupportedDatatypesValues() final = default;

    std::string name() const final
    {
        switch (value.getType())
        {
        case KvsValue::Type::i32:
            return "i32";
        case KvsValue::Type::u32:
            return "u32";
        case KvsValue::Type::i64:
            return "i64";
        case KvsValue::Type::u64:
            return "u64";
        case KvsValue::Type::f64:
            return "f64";
        case KvsValue::Type::Boolean:
            return "bool";
        case KvsValue::Type::String:
            return "str";
        case KvsValue::Type::Null:
            return "null";
        case KvsValue::Type::Array:
            return "arr";
        case KvsValue::Type::Object:
            return "obj";
        default:
            return "unknown";
        }
    }

    void run(const std::string &input) const final
    {
        // Create KVS instance with provided params.
        KvsParameters params{KvsParameters::from_json(input)};
        Kvs kvs = kvs_instance(params);

        // Set value
        kvs.set_value(name(), value);

        // Get value
        auto kvs_value = kvs.get_value(name());

        if (kvs_value.has_value())
        {
            info_log(name(), kvs_value_to_string(kvs_value.value()));
        }
        else
        {
            info_log(name() + "_error", std::string(kvs_value.error().Message()));
        }
    }

    // Factory functions for each value type scenario
    static Scenario::Ptr supported_datatypes_i32()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(-321));
    }

    static Scenario::Ptr supported_datatypes_u32()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(1234));
    }

    static Scenario::Ptr supported_datatypes_i64()
    {
        return std::make_shared<SupportedDatatypesValues>(
            KvsValue(static_cast<int64_t>(-123456789)));
    }

    static Scenario::Ptr supported_datatypes_u64()
    {
        return std::make_shared<SupportedDatatypesValues>(
            KvsValue(static_cast<uint64_t>(123456789)));
    }

    static Scenario::Ptr supported_datatypes_f64()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(-5432.1));
    }

    static Scenario::Ptr supported_datatypes_bool()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(true));
    }

    static Scenario::Ptr supported_datatypes_string()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue("example"));
    }

    static Scenario::Ptr supported_datatypes_array()
    {
        // Compose array value as in Rust
        std::unordered_map<std::string, KvsValue> obj = {
            {"sub-number", KvsValue(789)}};
        std::vector<KvsValue> arr =
            std::vector<KvsValue>{KvsValue(321.5),
                                  KvsValue(false),
                                  KvsValue("hello"),
                                  KvsValue(nullptr),
                                  KvsValue(std::vector<KvsValue>{}),
                                  KvsValue(obj)};
        return std::make_shared<SupportedDatatypesValues>(KvsValue(arr));
    }

    static Scenario::Ptr supported_datatypes_object()
    {
        std::unordered_map<std::string, KvsValue> obj = {
            {"sub-number", KvsValue(789)}};
        return std::make_shared<SupportedDatatypesValues>(KvsValue(obj));
    }

    static ScenarioGroup::Ptr value_types_group()
    {
        std::vector<Scenario::Ptr> scenarios = {
            supported_datatypes_i32(), supported_datatypes_u32(),
            supported_datatypes_i64(), supported_datatypes_u64(),
            supported_datatypes_f64(), supported_datatypes_bool(),
            supported_datatypes_string(), supported_datatypes_array(),
            supported_datatypes_object()};
        return std::make_shared<ScenarioGroupImpl>(
            "values", scenarios, std::vector<ScenarioGroup::Ptr>{});
    }
};

ScenarioGroup::Ptr supported_datatypes_group()
{
    std::vector<Scenario::Ptr> keys = {
        std::make_shared<SupportedDatatypesKeys>()};
    std::vector<ScenarioGroup::Ptr> groups = {
        SupportedDatatypesValues::value_types_group()};
    return std::make_shared<ScenarioGroupImpl>("supported_datatypes", keys,
                                               groups);
}

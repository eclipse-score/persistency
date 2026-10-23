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

#include "cit/supported_datatypes.h"

#include "helpers/kvs_instance.h"
#include "helpers/kvs_parameters.h"
#include <tracing.hpp>

#include "score/utility.hpp"
#include <iomanip>
#include <sstream>
#include <string>

using namespace score::mw::per::kvs;
using namespace score::json;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::supported_datatypes"};

void info_log_set_result(const std::string& keyname, bool set_ok)
{
    TRACING_INFO(kTargetName,
                 std::make_pair(std::string("key"), keyname),
                 std::make_pair(std::string("set_ok"), std::string(set_ok ? "true" : "false")));
}
void info_log_stored_key(const std::string& keyname)
{
    TRACING_INFO(kTargetName, std::make_pair(std::string("stored_key"), keyname));
}
void info_log(const std::string& name, const std::string& value)
{
    TRACING_INFO(kTargetName, std::make_pair(std::string(name), value));
}
void info_log(const std::string& key,
              const std::string& key_value,
              const std::string& value,
              const std::string& value_json)
{
    TRACING_INFO(
        kTargetName, std::make_pair(std::string("key"), key_value), std::make_pair(std::string("value"), value_json));
}
}  // namespace

class SupportedDatatypesKeys final : public Scenario
{
  public:
    ~SupportedDatatypesKeys() final = default;

    std::string name() const final
    {
        return "keys";
    }

    void run(const std::string& input) const final
    {
        // Create KVS instance with provided params.
        KvsParameters params{KvsParameters::from_json(input)};
        Kvs kvs = kvs_instance(params);

        // Try to set valid and invalid keys.
        // Null value is used - only key is used later on.
        std::vector<std::string> keys_to_check = {
            // Valid keys.
            "example",
            "snake_case_key",
            "kebab-case-key",
            "MixedCase0123",
            "a",
            std::string(32, 'k'),
            "with space",
            "dot.key",
            "slash/key",
            // Invalid keys.
            "",
            std::string(33, 'k'),
            // Non-ASCII characters.
            u8"emoji ✅❗😀",
            u8"greek ημα",
            // Non-ASCII characters, 32 bytes.
            u8"ąąąąąąąąąąąąąąąą",
        };
        for (const auto& s : keys_to_check)
        {
            auto set_result = kvs.set_value(s, KvsValue(nullptr));
            info_log_set_result(s, set_result.has_value());
        }

        // Log all keys stored in KVS.
        auto keys_in_kvs = kvs.get_all_keys();
        if (keys_in_kvs.has_value())
        {
            for (const auto& s : keys_in_kvs.value())
            {
                info_log_stored_key(s);
            }
        }
        else
        {
            info_log("get_all_keys_error", std::string(keys_in_kvs.error().Message()));
            throw keys_in_kvs.error();
        }
    }
};

class SupportedDatatypesValues final : public Scenario
{
  private:
    KvsValue value_;

    static std::string kvs_value_to_string(const KvsValue& v)
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
            {
                // Format with high precision, then remove trailing zeros and dot for minimal JSON representation.
                auto val = std::get<double>(v.getValue());
                std::ostringstream oss;
                oss << std::setprecision(15) << val;
                std::string s = oss.str();
                // Remove trailing zeros and dot if needed.
                if (auto dot = s.find('.'); dot != std::string::npos)
                {
                    // Find last non-zero digit after decimal point.
                    auto last_nonzero = s.find_last_not_of('0');
                    if (last_nonzero != std::string::npos && last_nonzero > dot)
                    {
                        s.erase(last_nonzero + 1);
                    }
                    // Remove dot if it's the last character.
                    if (s.back() == '.')
                        s.pop_back();
                }
                return s;
            }
            case KvsValue::Type::Boolean:
                return std::get<bool>(v.getValue()) ? "true" : "false";
            case KvsValue::Type::String:
                return "\"" + std::get<std::string>(v.getValue()) + "\"";
            case KvsValue::Type::Null:
                return "null";
            case KvsValue::Type::Array:
            {
                const auto& arr = std::get<KvsValue::Array>(v.getValue());
                std::string json = "[";
                for (size_t i = 0; i < arr.size(); ++i)
                {
                    const auto& elem = arr[i];
                    json += "{\"t\":\"" + SupportedDatatypesValues(elem).name() +
                            "\",\"v\":" + kvs_value_to_string(elem) + "}";
                    if (i + 1 < arr.size())
                        json += ",";
                }
                json += "]";
                return json;
            }
            case KvsValue::Type::Object:
            {
                const auto& obj = std::get<KvsValue::Object>(v.getValue());
                std::string json = "{";
                size_t count = 0;
                for (const auto& kv : obj)
                {
                    const auto& elem = kv.second;
                    json += "\"" + kv.first + "\":{\"t\":\"" + SupportedDatatypesValues(elem).name() +
                            "\",\"v\":" + kvs_value_to_string(elem) + "}";
                    if (++count < obj.size())
                        json += ",";
                }
                json += "}";
                return json;
            }
            default:
                return "null";
        }
    }

  public:
    explicit SupportedDatatypesValues(const KvsValue& v) : value_(v) {}

    ~SupportedDatatypesValues() final = default;

    std::string name() const final
    {
        switch (value_.getType())
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

    void run(const std::string& input) const final
    {
        // Create KVS instance with provided params.
        KvsParameters params{KvsParameters::from_json(input)};
        Kvs kvs = kvs_instance(params);

        // Set value.
        score::cpp::ignore = kvs.set_value(name(), value_);

        // Get value.
        auto kvs_value = kvs.get_value(name());

        if (kvs_value.has_value())
        {
            // Log key and value as separate fields for Python test compatibility.
            info_log("key",
                     name(),
                     "value",
                     std::string("{\"t\":\"" + name() + "\",\"v\":" + kvs_value_to_string(kvs_value.value()) + "}"));
        }
        else
        {
            info_log(name() + "_error", std::string(kvs_value.error().Message()));
        }
    }

    // Factory functions for each value type scenario.
    static Scenario::Ptr supported_datatypes_i32()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(static_cast<int32_t>(-321)));
    }

    static Scenario::Ptr supported_datatypes_u32()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(static_cast<uint32_t>(1234)));
    }

    static Scenario::Ptr supported_datatypes_i64()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(static_cast<int64_t>(-123456789)));
    }

    static Scenario::Ptr supported_datatypes_u64()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(static_cast<uint64_t>(123456789)));
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
        // Compose array value as in Rust.
        std::unordered_map<std::string, KvsValue> obj = {{"sub-number", KvsValue(789.0)}};
        std::vector<KvsValue> arr = std::vector<KvsValue>{KvsValue(321.5),
                                                          KvsValue(false),
                                                          KvsValue("hello"),
                                                          KvsValue(nullptr),
                                                          KvsValue(std::vector<KvsValue>{}),
                                                          KvsValue(obj)};
        return std::make_shared<SupportedDatatypesValues>(KvsValue(arr));
    }

    static Scenario::Ptr supported_datatypes_object()
    {
        std::unordered_map<std::string, KvsValue> obj = {{"sub-number", KvsValue(789.0)}};
        return std::make_shared<SupportedDatatypesValues>(KvsValue(obj));
    }

    static ScenarioGroup::Ptr value_types_group()
    {
        std::vector<Scenario::Ptr> scenarios = {supported_datatypes_i32(),
                                                supported_datatypes_u32(),
                                                supported_datatypes_i64(),
                                                supported_datatypes_u64(),
                                                supported_datatypes_f64(),
                                                supported_datatypes_bool(),
                                                supported_datatypes_string(),
                                                supported_datatypes_array(),
                                                supported_datatypes_object()};
        return std::make_shared<ScenarioGroupImpl>("values", scenarios, std::vector<ScenarioGroup::Ptr>{});
    }
};

ScenarioGroup::Ptr supported_datatypes_group()
{
    std::vector<Scenario::Ptr> keys = {std::make_shared<SupportedDatatypesKeys>()};
    std::vector<ScenarioGroup::Ptr> groups = {SupportedDatatypesValues::value_types_group()};
    return std::make_shared<ScenarioGroupImpl>("supported_datatypes", keys, groups);
}

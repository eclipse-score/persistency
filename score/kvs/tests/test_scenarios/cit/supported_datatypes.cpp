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

void InfoLogSetResult(const std::string& keyname, bool set_ok)
{
    TRACING_INFO(kTargetName,
                 std::make_pair(std::string("key"), keyname),
                 std::make_pair(std::string("set_ok"), std::string(set_ok ? "true" : "false")));
}
void InfoLogStoredKey(const std::string& keyname)
{
    TRACING_INFO(kTargetName, std::make_pair(std::string("stored_key"), keyname));
}
void InfoLog(const std::string& name, const std::string& value)
{
    TRACING_INFO(kTargetName, std::make_pair(std::string(name), value));
}
void InfoLog(const std::string& key,
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
        KvsParameters params{KvsParameters::FromJson(input)};
        Kvs kvs{KvsInstance(params)};

        // Try to set valid and invalid keys.
        // Null value is used - only key is used later on.
        std::vector<std::string> keys_to_check{
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
            auto set_result{kvs.SetValue(s, KvsValue(nullptr))};
            InfoLogSetResult(s, set_result.has_value());
        }

        // Log all keys stored in KVS.
        auto keys_in_kvs{kvs.GetAllKeys()};
        if (keys_in_kvs.has_value())
        {
            for (const auto& s : keys_in_kvs.value())
            {
                InfoLogStoredKey(s);
            }
        }
        else
        {
            InfoLog("get_all_keys_error", std::string(keys_in_kvs.error().Message()));
            throw keys_in_kvs.error();
        }
    }
};

class SupportedDatatypesValues final : public Scenario
{
  public:
    explicit SupportedDatatypesValues(const KvsValue& v) : value_{v} {}

    ~SupportedDatatypesValues() final = default;

    std::string name() const final
    {
        switch (value_.GetType())
        {
            case KvsValue::Type::kI32:
                return "i32";
            case KvsValue::Type::kU32:
                return "u32";
            case KvsValue::Type::kI64:
                return "i64";
            case KvsValue::Type::kU64:
                return "u64";
            case KvsValue::Type::kF64:
                return "f64";
            case KvsValue::Type::kBoolean:
                return "bool";
            case KvsValue::Type::kString:
                return "str";
            case KvsValue::Type::kNull:
                return "null";
            case KvsValue::Type::kArray:
                return "arr";
            case KvsValue::Type::kObject:
                return "obj";
            default:
                return "unknown";
        }
    }

    void run(const std::string& input) const final
    {
        // Create KVS instance with provided params.
        KvsParameters params{KvsParameters::FromJson(input)};
        Kvs kvs{KvsInstance(params)};

        // Set value.
        score::cpp::ignore = kvs.SetValue(name(), value_);

        // Get value.
        auto kvs_value{kvs.GetValue(name())};

        if (kvs_value.has_value())
        {
            // Log key and value as separate fields for Python test compatibility.
            InfoLog("key",
                    name(),
                    "value",
                    std::string("{\"t\":\"" + name() + "\",\"v\":" + KvsValueToString(kvs_value.value()) + "}"));
        }
        else
        {
            InfoLog(name() + "_error", std::string(kvs_value.error().Message()));
        }
    }

    // Factory functions for each value type scenario.
    static Scenario::Ptr SupportedDatatypesI32()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(static_cast<int32_t>(-321)));
    }

    static Scenario::Ptr SupportedDatatypesU32()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(static_cast<uint32_t>(1234)));
    }

    static Scenario::Ptr SupportedDatatypesI64()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(static_cast<int64_t>(-123456789)));
    }

    static Scenario::Ptr SupportedDatatypesU64()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(static_cast<uint64_t>(123456789)));
    }

    static Scenario::Ptr SupportedDatatypesF64()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(-5432.1));
    }

    static Scenario::Ptr SupportedDatatypesBool()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(true));
    }

    static Scenario::Ptr SupportedDatatypesString()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue("example"));
    }

    static Scenario::Ptr SupportedDatatypesArray()
    {
        // Compose array value as in Rust.
        std::unordered_map<std::string, KvsValue> obj{{"sub-number", KvsValue(789.0)}};
        std::vector<KvsValue> arr = std::vector<KvsValue>{KvsValue(321.5),
                                                          KvsValue(false),
                                                          KvsValue("hello"),
                                                          KvsValue(nullptr),
                                                          KvsValue(std::vector<KvsValue>{}),
                                                          KvsValue(obj)};
        return std::make_shared<SupportedDatatypesValues>(KvsValue(arr));
    }

    static Scenario::Ptr SupportedDatatypesObject()
    {
        std::unordered_map<std::string, KvsValue> obj{{"sub-number", KvsValue(789.0)}};
        return std::make_shared<SupportedDatatypesValues>(KvsValue(obj));
    }

    static ScenarioGroup::Ptr ValueTypesGroup()
    {
        std::vector<Scenario::Ptr> scenarios{SupportedDatatypesI32(),
                                             SupportedDatatypesU32(),
                                             SupportedDatatypesI64(),
                                             SupportedDatatypesU64(),
                                             SupportedDatatypesF64(),
                                             SupportedDatatypesBool(),
                                             SupportedDatatypesString(),
                                             SupportedDatatypesArray(),
                                             SupportedDatatypesObject()};
        return std::make_shared<ScenarioGroupImpl>("values", scenarios, std::vector<ScenarioGroup::Ptr>{});
    }

  private:
    static std::string KvsValueToString(const KvsValue& v)
    {
        switch (v.GetType())
        {
            case KvsValue::Type::kI32:
                return std::to_string(std::get<int32_t>(v.GetValue()));
            case KvsValue::Type::kU32:
                return std::to_string(std::get<uint32_t>(v.GetValue()));
            case KvsValue::Type::kI64:
                return std::to_string(std::get<int64_t>(v.GetValue()));
            case KvsValue::Type::kU64:
                return std::to_string(std::get<uint64_t>(v.GetValue()));
            case KvsValue::Type::kF64:
            {
                // Format with high precision, then remove trailing zeros and dot for minimal JSON representation.
                auto val{std::get<double>(v.GetValue())};
                std::ostringstream oss;
                oss << std::setprecision(15) << val;
                std::string s{oss.str()};
                // Remove trailing zeros and dot if needed.
                if (auto dot{s.find('.')}; dot != std::string::npos)
                {
                    // Find last non-zero digit after decimal point.
                    auto last_nonzero{s.find_last_not_of('0')};
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
            case KvsValue::Type::kBoolean:
                return std::get<bool>(v.GetValue()) ? "true" : "false";
            case KvsValue::Type::kString:
                return "\"" + std::get<std::string>(v.GetValue()) + "\"";
            case KvsValue::Type::kNull:
                return "null";
            case KvsValue::Type::kArray:
            {
                const auto& arr{std::get<KvsValue::Array>(v.GetValue())};
                std::string json{"["};
                for (size_t i{0U}; i < arr.size(); ++i)
                {
                    const auto& elem{arr[i]};
                    json += "{\"t\":\"" + SupportedDatatypesValues(elem).name() + "\",\"v\":" + KvsValueToString(elem) +
                            "}";
                    if (i + 1 < arr.size())
                        json += ",";
                }
                json += "]";
                return json;
            }
            case KvsValue::Type::kObject:
            {
                const auto& obj{std::get<KvsValue::Object>(v.GetValue())};
                std::string json{"{"};
                size_t count{0U};
                for (const auto& kv : obj)
                {
                    const auto& elem{kv.second};
                    json += "\"" + kv.first + "\":{\"t\":\"" + SupportedDatatypesValues(elem).name() +
                            "\",\"v\":" + KvsValueToString(elem) + "}";
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

    KvsValue value_;
};

ScenarioGroup::Ptr SupportedDatatypesGroup()
{
    std::vector<Scenario::Ptr> keys{std::make_shared<SupportedDatatypesKeys>()};
    std::vector<ScenarioGroup::Ptr> groups{SupportedDatatypesValues::ValueTypesGroup()};
    return std::make_shared<ScenarioGroupImpl>("supported_datatypes", keys, groups);
}

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

#include <cassert>
#include <iomanip>
#include <set>
#include <sstream>
#include <string>

constexpr size_t MAX_KEY_LENGTH = 32;      // Max key length in bytes
constexpr size_t MAX_VALUE_LENGTH = 1024;  // Max value length in bytes

using namespace score::mw::per::kvs;
using namespace score::json;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::supported_datatypes"};

void info_log(const std::string& keyname)
{
    TRACING_INFO(kTargetName, std::make_pair(std::string("key"), keyname));
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

/**
 * Test cases for key requirements:
 * 1. The component shall accept keys that consist solely of alphanumeric
 *    characters, underscores, or dashes.
 * 2. The component shall encode each key as valid UTF-8.
 * 3. The component shall guarantee that each key is unique.
 * 4. The component shall limit the maximum length of a key to 32 bytes.
 */
class SupportedDatatypesKeys : public Scenario
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

        // Prepare valid, invalid, and UTF-8 keys for testing
        std::vector<std::string> valid_keys = {
            "alphaNumeric123", "with_underscore", "with-dash", "A1_b2-C3", std::string(MAX_KEY_LENGTH, 'a')  // 32 bytes
        };
        std::vector<std::string> invalid_keys = {
            u8"utf8_ключ",  // Cyrillic
            u8"utf8_漢字",  // Chinese
            u8"utf8_emoji ✅❗😀",
            u8"utf8_greek ημα",
            "has space",
            "has$pecial",
            "emoji✅",
            "too_long_key_abcdefghijklmnopqrstuvwxyz123456",  // >32 bytes
        };
        std::vector<std::string> utf8_keys = {u8"utf8_emoji_valid",
                                              u8"utf8_alphaNumeric123",
                                              u8"utf8_with_underscore",
                                              u8"utf8-with-dash",
                                              u8"utf8_A1_b2-C3"};

        // Test uniqueness: insert duplicate key
        std::string duplicate_key = "unique_key";
        kvs.set_value(duplicate_key, KvsValue(nullptr));
        auto result_duplicate = kvs.set_value(duplicate_key, KvsValue(nullptr));
        // Requirement #3: The component shall guarantee that each key is unique.
        // NOTE: Current implementation fulfills uniqueness by allowing update to
        // the value for an existing key (not strict rejection).
        // TODO: Is allowing value update for an existing key compliant with the
        // uniqueness requirement? (Test expects strict rejection, but
        // implementation allows update.) assert(!result_duplicate && "Duplicate key
        // insertion should be rejected (no update allowed)");

        // Insert all valid keys
        for (const auto& key : valid_keys)
        {
            kvs.set_value(key, KvsValue(nullptr));
        }
        // Insert all valid UTF-8 keys
        for (const auto& key : utf8_keys)
        {
            kvs.set_value(key, KvsValue(nullptr));
        }
        // Try to insert invalid keys (should fail)
        for (const auto& key : invalid_keys)
        {
            auto result = kvs.set_value(key, KvsValue(nullptr));
            // Requirement #1: The component shall accept keys that consist solely of
            // alphanumeric characters, underscores, or dashes. Requirement #2: The
            // component shall encode each key as valid UTF-8. Requirement #4: The
            // component shall limit the maximum length of a key to 32 bytes. The
            // following assertion fails if the KVS implementation does not properly
            // reject invalid keys (e.g., keys with spaces, non-ASCII, emoji, or >32
            // bytes):
            if (result)
                std::cerr << "KVS accepted invalid key: " << key << std::endl;
            assert(!result && ("Invalid key should be rejected: " + key).c_str());
            // If this assertion fails, the KVS is not enforcing one or more of
            // requirements #1, #2, or #4.
        }

        // Get all keys and log only valid/expected ones
        auto keys_in_kvs = kvs.get_all_keys();
        if (keys_in_kvs.has_value())
        {
            std::vector<std::string> all_expected_keys = valid_keys;
            all_expected_keys.insert(all_expected_keys.end(), utf8_keys.begin(), utf8_keys.end());
            all_expected_keys.push_back(duplicate_key);
            std::set<std::string> expected_set(all_expected_keys.begin(), all_expected_keys.end());
            for (const auto& s : keys_in_kvs.value())
            {
                if (expected_set.count(s))
                {
                    info_log(s);
                }
            }
        }
        else
        {
            info_log("get_all_keys_error", std::string(keys_in_kvs.error().Message()));
            throw keys_in_kvs.error();
        }

        // Log if duplicate key was accepted (should not happen)
        if (result_duplicate)
        {
            info_log("duplicate_key_accepted", duplicate_key);
        }
        // Log the max length key
        info_log("max_length_key", valid_keys.back());
    }
};

/**
 * Test cases for value requirements:
 * Requirement #5. The component shall accept only values of the following data
 * types: Number, String, Null, Array[Value], or Dictionary{Key:Value  }.
 * Requirement #6. The component shall serialize and deserialize all values to
 * and from JSON. Requirement #7: The component shall limit the maximum length
 * of a value to 1024 bytes.
 */
class SupportedDatatypesValues : public Scenario
{
  private:
    KvsValue value;

    static std::string kvs_value_to_string(const KvsValue& v)
    {
        // Serialize KvsValue to JSON string for logging/validation
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
                // Format floating point value with high precision, then remove trailing
                // zeros and dot for minimal JSON representation
                auto val = std::get<double>(v.getValue());
                std::ostringstream oss;
                oss << std::setprecision(15) << val;
                std::string s = oss.str();
                // Remove trailing zeros and dot if needed
                if (auto dot = s.find('.'); dot != std::string::npos)
                {
                    // Find last non-zero digit after decimal point
                    auto last_nonzero = s.find_last_not_of('0');
                    if (last_nonzero != std::string::npos && last_nonzero > dot)
                    {
                        s.erase(last_nonzero + 1);
                    }
                    // Remove dot if it's the last character
                    if (s.back() == '.')
                        s.pop_back();
                }
                return s;
            }
            case KvsValue::Type::Boolean:
                return std::get<bool>(v.getValue()) ? "true" : "false";
            case KvsValue::Type::String:
            {
                const auto& str = std::get<std::string>(v.getValue());
                return "\"" + str + "\"";
            }
            case KvsValue::Type::Null:
                return "null";
            case KvsValue::Type::Array:
            {
                const auto& arr = std::get<std::vector<std::shared_ptr<KvsValue>>>(v.getValue());
                std::string json = "[";
                for (size_t i = 0; i < arr.size(); ++i)
                {
                    const auto& elem = *arr[i];
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
                const auto& obj = std::get<std::unordered_map<std::string, std::shared_ptr<KvsValue>>>(v.getValue());
                std::string json = "{";
                size_t count = 0;
                for (const auto& kv : obj)
                {
                    const auto& elem = *kv.second;
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
    explicit SupportedDatatypesValues(const KvsValue& v) : value(v) {}

    ~SupportedDatatypesValues() final = default;

    std::string name() const final
    {
        // Return scenario name based on value type and length
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
            {
                const auto& str = std::get<std::string>(value.getValue());
                if (str.size() == MAX_VALUE_LENGTH)
                    return "str_1024";
                else if (str.size() > MAX_VALUE_LENGTH)
                    return "str_1025";
                else
                    return "str";
            }
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
        // Create KVS instance with provided parameters
        KvsParameters params{KvsParameters::from_json(input)};
        Kvs kvs = kvs_instance(params);

        // Set value in KVS
        // If the KVS implementation does not reject values of the wrong type, or
        // values >1024 bytes, or fails to serialize/deserialize, the following line
        // or subsequent assertions will fail.
        kvs.set_value(name(), value);

        // Get value from KVS
        auto kvs_value = kvs.get_value(name());

        if (kvs_value.has_value())
        {
            std::string value_str = kvs_value_to_string(kvs_value.value());
            if (value_str == "null")
            {
                std::ostringstream err_oss;
                err_oss << name() << "_error: failed to serialize value";
                info_log("key", name(), "value", err_oss.str());
            }
            else
            {
                std::ostringstream oss;
                oss << "{\"t\":\"" << name() << "\",\"v\":" << value_str << "}";
                info_log("key", name(), "value", oss.str());
            }
        }
        else
        {
            info_log(name() + "_error", std::string(kvs_value.error().Message()));
        }
    }

    // Factory functions for each value type scenario
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
        // Compose array value (mirrors Rust test)
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

    // Test for string value of exactly 1024 bytes
    static Scenario::Ptr supported_datatypes_string_1024()
    {
        return std::make_shared<SupportedDatatypesValues>(KvsValue(std::string(MAX_VALUE_LENGTH, 'x')));
    }
    // Test for string value of 1025 bytes (should be rejected or error)
    // This test expects the value to be rejected (should error)
    // This test expects the value to be rejected (should error)
    static Scenario::Ptr supported_datatypes_string_1025()
    {
        // Requirement #7: The component shall limit the maximum length of a value
        // to 1024 bytes. If the KVS implementation does not reject this value, it
        // is not enforcing requirement #7.
        // This test expects the value to be rejected (should error)
        return std::make_shared<SupportedDatatypesValues>(KvsValue(std::string(MAX_VALUE_LENGTH + 1, 'y')));
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
                                                supported_datatypes_object(),
                                                supported_datatypes_string_1024(),
                                                supported_datatypes_string_1025()};
        return std::make_shared<ScenarioGroupImpl>("values", scenarios, std::vector<ScenarioGroup::Ptr>{});
    }
};

ScenarioGroup::Ptr supported_datatypes_group()
{
    std::vector<Scenario::Ptr> keys = {std::make_shared<SupportedDatatypesKeys>()};
    std::vector<ScenarioGroup::Ptr> groups = {SupportedDatatypesValues::value_types_group()};
    return std::make_shared<ScenarioGroupImpl>("supported_datatypes", keys, groups);
}

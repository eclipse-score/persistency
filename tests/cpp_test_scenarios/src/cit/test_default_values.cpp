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

#include "test_default_values.hpp"
#include "helpers/kvs_instance.hpp"
#include "helpers/kvs_parameters.hpp"
#include "tracing.hpp"
#include <sstream>

using score::mw::per::kvs::KvsValue;

namespace test_default_values
{

    const std::string kTargetName{"cpp_test_scenarios::cit::default_values"};

    /**
     * Helper to log key/value state in a format parsable by Python tests.
     *
     * @param key The key being queried or modified in the KVS.
     * @param value_is_default String encoding whether the current value matches the
     * default ("Ok(true)", "Ok(false)", or error string).
     * @param default_value String encoding the default value for the key (e.g.,
     * "Ok(F64(...))" or error string).
     * @param current_value String encoding the current value for the key (e.g.,
     * "Ok(F64(...))" or error string).
     *
     * This function emits logs in a structured format so that the Python test suite
     * can parse and validate scenario output.
     */
    static void info_log(const std::string &key,
                         const std::string &value_is_default,
                         const std::string &default_value,
                         const std::string &current_value)
    {
        TRACING_INFO(kTargetName, std::pair{std::string{"key"}, key},
                     std::pair{std::string{"value_is_default"}, value_is_default},
                     std::pair{std::string{"default_value"}, default_value},
                     std::pair{std::string{"current_value"}, current_value});
    }

    /**
     * Overload of info_log for logging key/value state with a typed current value.
     *
     * @tparam T The type of the current value to log.
     * @param key The key being queried or modified in the KVS.
     * @param value_is_default Boolean indicating whether the current value matches the default.
     * @param current_value The current value for the key, of type T.
     *
     * This function emits logs in a structured format so that the Python test suite
     * can parse and validate scenario output. Unlike the string overload, this version
     * logs the current value as a typed parameter and omits the default value.
     */
    template <typename T>
    static void info_log(const std::string &key, const bool value_is_default,
                         T current_value)
    {
        TRACING_INFO(kTargetName, std::pair{std::string{"key"}, key},
                     std::pair{std::string{"value_is_default"}, value_is_default},
                     std::pair{std::string{"current_value"}, current_value});
    }

    std::string DefaultValuesScenario::name() const { return "default_values"; }
    void DefaultValuesScenario::run(const std::string &input) const
    {

        using namespace score::mw::per::kvs;
        std::string key = "test_number";
        auto params = map_to_params(input);
        {
            auto kvs = kvs_instance(params);
            // First check: log initial state before any set_value
            auto get_value_is_default = kvs.has_default_value(key);
            auto get_default_result = kvs.get_default_value(key);
            auto get_value_result = kvs.get_value(key);
            std::string value_is_default;
            std::string default_value;
            std::string current_value;

            // Default value
            if (!get_default_result.has_value() ||
                get_default_result.value().getType() != KvsValue::Type::f64 ||
                !std::holds_alternative<double>(
                    get_default_result.value().getValue()))
            {
                default_value = "Err(KeyNotFound)";
            }
            else
            {
                std::ostringstream oss;
                oss.precision(1);
                oss << std::fixed
                    << std::get<double>(get_default_result.value().getValue());
                default_value = "Ok(F64(" + oss.str() + "))";
            }

            // Current value
            if (!get_value_result.has_value() ||
                get_value_result.value().getType() != KvsValue::Type::f64 ||
                !std::holds_alternative<double>(
                    get_value_result.value().getValue()))
            {
                current_value = "Err(KeyNotFound)";
            }
            else
            {
                std::ostringstream oss;
                oss.precision(1);
                oss << std::fixed
                    << std::get<double>(get_value_result.value().getValue());
                current_value = "Ok(F64(" + oss.str() + "))";
            }

            // Has default value
            value_is_default = get_value_is_default.has_value()
                                   ? (get_value_is_default.value() ? "Ok(true)" : "Ok(false)")
                                   : "Err(KeyNotFound)";

            info_log(key, value_is_default, default_value, current_value);

            auto set_result = kvs.set_value(key, KvsValue{432.1});
            if (!set_result)
                throw set_result.error();
            kvs.flush();
        }

        {
            // Second check: log after set_value and flush
            // - value_is_default: Ok(true) if value == default, Ok(false) if not,
            // Err(KeyNotFound) if default missing
            auto kvs = kvs_instance(params);
            auto get_value_is_default = kvs.has_default_value(key);
            auto get_default_result = kvs.get_default_value(key);
            auto get_value_result = kvs.get_value(key);
            std::string value_is_default;
            std::string default_value;
            std::string current_value;
            bool get_default_ok = get_default_result.has_value();
            bool get_value_ok = get_value_result.has_value();
            const KvsValue *def_val =
                get_default_ok ? &get_default_result.value() : nullptr;
            const KvsValue *cur_val =
                get_value_ok ? &get_value_result.value() : nullptr;

            // Has default value
            value_is_default = get_value_is_default.has_value()
                                   ? (get_value_is_default.value() ? "Ok(true)" : "Ok(false)")
                                   : "Err(KeyNotFound)";

            // Format default_value for log
            if (get_default_ok && def_val->getType() == KvsValue::Type::f64)
            {
                std::ostringstream oss;
                oss.precision(1);
                oss << std::fixed << std::get<double>(def_val->getValue());
                default_value = "Ok(F64(" + oss.str() + "))";
            }
            else if (get_default_ok)
            {
                default_value =
                    "Err(UnexpectedType:" +
                    std::to_string(static_cast<int>(def_val->getType())) + ")";
            }
            else
            {
                default_value = "Err(KeyNotFound)";
            }

            // Format current_value for log
            if (get_value_ok && cur_val->getType() == KvsValue::Type::f64)
            {
                std::ostringstream oss;
                oss.precision(1);
                oss << std::fixed << std::get<double>(cur_val->getValue());
                current_value = "Ok(F64(" + oss.str() + "))";
            }
            else if (get_value_ok)
            {
                current_value =
                    "Err(UnexpectedType:" +
                    std::to_string(static_cast<int>(cur_val->getType())) + ")";
            }
            else
            {
                current_value = "Err(KeyNotFound)";
            }

            info_log(key, value_is_default, default_value,
                     current_value); // Log after set/flush
        }
    }

    std::string RemoveKeyScenario::name() const { return "remove_key"; }
    void RemoveKeyScenario::run(const std::string &input) const
    {
        using namespace score::mw::per::kvs;
        std::string key = "test_number";
        auto params = map_to_params(input);
        auto kvs = kvs_instance(params);

        auto get_value_is_default = kvs.has_default_value(key);
        auto get_default = kvs.get_default_value(key);
        auto get_value = kvs.get_value(key);
        std::string value_is_default;
        std::string default_value;
        std::string current_value;

        // Default value
        if (!get_default || get_default->getType() != KvsValue::Type::f64 ||
            !std::holds_alternative<double>(get_default->getValue()))
        {
            default_value = "Err(KeyNotFound)";
        }
        else
        {
            std::ostringstream oss;
            oss.precision(1);
            oss << std::fixed << std::get<double>(get_default->getValue());
            default_value = "Ok(F64(" + oss.str() + "))";
        }

        // Current value
        if (!get_value || get_value->getType() != KvsValue::Type::f64 ||
            !std::holds_alternative<double>(get_value->getValue()))
        {
            current_value = "Err(KeyNotFound)";
        }
        else
        {
            std::ostringstream oss;
            oss.precision(1);
            oss << std::fixed << std::get<double>(get_value->getValue());
            current_value = "Ok(F64(" + oss.str() + "))";
        }

        // value_is_default
        value_is_default = get_value_is_default.has_value()
                               ? (get_value_is_default.value() ? "Ok(true)" : "Ok(false)")
                               : "Err(KeyNotFound)";

        info_log(key, value_is_default, default_value, current_value);

        auto set_result = kvs.set_value(key, KvsValue{432.1});
        if (!set_result)
            throw set_result.error();
        get_value = kvs.get_value(key);
        get_value_is_default = kvs.has_default_value(key);
        // Second check: log after set_value
        // - value_is_default: Ok(true) if value == default, Ok(false) if not
        value_is_default = get_value_is_default.has_value()
                               ? (get_value_is_default.value() ? "Ok(true)" : "Ok(false)")
                               : "Err(KeyNotFound)";
        // Format current_value for log
        if (get_value && get_value->getType() == KvsValue::Type::f64)
        {
            std::ostringstream oss;
            oss.precision(1);
            oss << std::fixed << std::get<double>(get_value->getValue());
            current_value = "Ok(F64(" + oss.str() + "))";
        }
        else
        {
            current_value = "Err(KeyNotFound)";
        }

        info_log(key, value_is_default, default_value,
                 current_value); // Log after set

        auto remove_result = kvs.remove_key(key);
        if (!remove_result)
            throw remove_result.error();
        get_value = kvs.get_value(key);

        // Third check: log after remove_key
        // - value_is_default: Err(KeyNotFound) if default missing, Ok(true) if
        // value
        // == default, Ok(false) otherwise
        get_value_is_default = kvs.has_default_value(key);

        value_is_default = get_value_is_default.has_value()
                               ? (get_value_is_default.value() ? "Ok(true)" : "Ok(false)")
                               : "Err(KeyNotFound)";

        // Format current_value for log
        if (get_value && get_value->getType() == KvsValue::Type::f64)
        {
            std::ostringstream oss;
            oss.precision(1);
            oss << std::fixed << std::get<double>(get_value->getValue());
            current_value = "Ok(F64(" + oss.str() + "))";
        }
        else
        {
            current_value = "Err(KeyNotFound)";
        }

        info_log(key, value_is_default, default_value,
                 current_value); // Log after remove
    }

    std::string ResetAllKeysScenario::name() const { return "reset_all_keys"; }
    void ResetAllKeysScenario::run(const std::string &input) const
    {
        using namespace score::mw::per::kvs;
        const int num_values = 5;
        auto params = map_to_params(input);
        auto kvs = kvs_instance(params);

        std::vector<std::pair<std::string, double>> key_values;
        for (int i = 0; i < num_values; ++i)
        {
            key_values.emplace_back("test_number_" + std::to_string(i), 123.4 * i);
        }

        for (const auto &[key, value] : key_values)
        {
            {
                const bool value_is_default = kvs.has_default_value(key).value();
                const double current_value =
                    std::get<double>((*kvs.get_value(key)).getValue());

                info_log(key, value_is_default, current_value);
            }

            kvs.set_value(key, KvsValue{value});

            {
                const bool value_is_default = kvs.has_default_value(key).value();
                const double current_value =
                    std::get<double>((*kvs.get_value(key)).getValue());

                info_log(key, value_is_default, current_value);
            }
        }

        kvs.reset();
        for (const auto &[key, _] : key_values)
        {
            const bool value_is_default = kvs.has_default_value(key).value();
            const double current_value =
                std::get<double>((*kvs.get_value(key)).getValue());

            info_log(key, value_is_default, current_value);
        }
    }

    std::string ResetSingleKeyScenario::name() const { return "reset_single_key"; }
    void ResetSingleKeyScenario::run(const std::string &input) const
    {
        using namespace score::mw::per::kvs;
        int num_values = 5;
        int reset_index = 2;
        auto params = map_to_params(input);
        auto kvs = kvs_instance(params);

        std::vector<std::pair<std::string, double>> key_values;
        for (int i = 0; i < num_values; ++i)
        {
            key_values.emplace_back("test_number_" + std::to_string(i), 123.4 * i);
        }

        for (const auto &[key, value] : key_values)
        {
            {
                const bool value_is_default = kvs.has_default_value(key).value();
                const double current_value =
                    std::get<double>((*kvs.get_value(key)).getValue());

                info_log(key, value_is_default, current_value);
            }
            // Set value.
            kvs.set_value(key, KvsValue{value});

            // Get value parameters after set.
            {
                const bool value_is_default = kvs.has_default_value(key).value();
                const double current_value =
                    std::get<double>((*kvs.get_value(key)).getValue());

                info_log(key, value_is_default, current_value);
            }
        }

        // Reset single key.
        auto reset_result = kvs.reset_key(key_values[reset_index].first);
        if (!reset_result)
            throw reset_result.error();

        // Use KVS APIs to get value_is_default and current_value after reset
        for (const auto &[key, value] : key_values)
        {
            const bool value_is_default = kvs.has_default_value(key).value();
            const double current_value =
                std::get<double>((*kvs.get_value(key)).getValue());
            info_log(key, value_is_default, current_value);
        }
    }

    std::string ChecksumScenario::name() const { return "checksum"; }
    void ChecksumScenario::run(const std::string &input) const
    {
        using namespace score::mw::per::kvs;
        auto params = map_to_params(input);
        std::string kvs_path, hash_path;
        try
        {
            auto kvs = kvs_instance(params);
            kvs.flush();

            // Get kvs_path
            auto kvs_path_res = kvs.get_kvs_filename(0);
            if (kvs_path_res.has_value())
            {
                kvs_path = static_cast<std::string>(kvs_path_res.value());
            }
            else
            {
                kvs_path = "";
            }

            // Get hash_path
            auto hash_path_res = kvs.get_hash_filename(0);
            if (hash_path_res.has_value())
            {
                hash_path = static_cast<std::string>(hash_path_res.value());
            }
            else
            {
                hash_path = "";
            }
        }
        catch (const std::exception &e)
        {
            kvs_path = "";
            hash_path = "";
        }

        // Log using Rust-compatible field names for Python test parsing
        TRACING_INFO(kTargetName, std::pair{std::string{"kvs_path"}, kvs_path},
                     std::pair{std::string{"hash_path"}, hash_path});
    }

    // Default values group
    ScenarioGroup::Ptr create_default_values_group()
    {
        return ScenarioGroup::Ptr{
            new ScenarioGroupImpl{"default_values",
                                  {std::make_shared<DefaultValuesScenario>(),
                                   std::make_shared<RemoveKeyScenario>(),
                                   std::make_shared<ResetAllKeysScenario>(),
                                   std::make_shared<ResetSingleKeyScenario>(),
                                   std::make_shared<ChecksumScenario>()},
                                  {}}};
    }
} // namespace test_default_values

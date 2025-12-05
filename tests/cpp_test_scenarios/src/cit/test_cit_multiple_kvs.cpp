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

#include "test_cit_multiple_kvs.hpp"

#include "helpers/kvs_instance.hpp"
#include "helpers/kvs_parameters.hpp"
#include "tracing.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

// Common function to extract double from variant
template <typename Variant>
double safe_extract_double(const Variant &var)
{
    return std::visit([](auto &&v) -> double
                      {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_arithmetic_v<T>)
            return static_cast<double>(v);
        throw std::runtime_error("Unexpected value type for key 'number'"); }, var);
}

namespace test_multiple_kvs
{

    using namespace score::mw::per::kvs;
    const std::string kTargetName{"cpp_test_scenarios::multiple_kvs"};

    std::string extract_kvs_param_json(const score::json::Any &any)
    {

        try
        {
            const auto &obj = any.As<score::json::Object>().value().get();

            auto it = obj.find("kvs_parameters");
            if (it == obj.end())
            {
                std::cout << "[DEBUG] FINDING Error: Missing 'kvs_parameters' in input\n";
                throw std::runtime_error("Missing 'kvs_parameters' in input");
            }

            auto str_val = it->second.As<std::string>();
            if (str_val.has_value())
            {
                std::string s = str_val.value().get();

                return "{\"kvs_parameters\":" + s + "}";
            }
            // If it's an object, serialize recursively
            auto obj_val = it->second.As<score::json::Object>();
            if (obj_val.has_value())
            {
                const auto &inner_obj = obj_val.value().get();
                std::ostringstream oss;
                oss << "{";
                bool first = true;
                for (const auto &inner_kv : inner_obj)
                {
                    if (!first)
                        oss << ",";
                    first = false;
                    // Print key as string
                    {
                        auto sv = inner_kv.first.GetAsStringView();
                        oss << "\"" << std::string(sv.data(), sv.size()) << "\":";
                    }
                    // Try to print value as string or number
                    auto sval = inner_kv.second.As<std::string>();
                    if (sval.has_value())
                    {
                        oss << '"' << sval.value().get() << '"';
                    }
                    else
                    {
                        auto ival = inner_kv.second.As<int64_t>();
                        if (ival.has_value())
                        {
                            oss << ival.value();
                        }
                        else
                        {
                            auto dval = inner_kv.second.As<double>();
                            if (dval.has_value())
                            {
                                oss << std::fixed << std::setprecision(1) << dval.value();
                            }
                            else
                            {
                                oss << "null";
                            }
                        }
                    }
                }
                oss << "}";
                std::string json_str = oss.str();

                return "{\"kvs_parameters\":" + json_str + "}";
            }
            // Fallback: not a string or object

            return "{\"kvs_parameters\":null}";
        }
        catch (const std::exception &e)
        {

            throw e;
        }
    }

    template <typename T>
    static void info_log(const std::string &instance, const std::string &keyname, T value)
    {
        std::cout << "Value is :" << value << std::endl;
        // ToDo: TRACING_INFO is not setting the precision of floating point to 1 decimal .
        //  Change either global subsrciber in tracing or python test cases.
        TRACING_INFO(kTargetName, std::pair{std::string{"instance"}, instance},
                     std::pair{std::string{"key"}, keyname},
                     std::pair{std::string{"value"}, value});
    }

    std::string MultipleInstanceIds::name() const { return "multiple_instance_ids"; }
    void MultipleInstanceIds::run(const std::string &input) const
    {

        const std::string keyname = "number";
        const double value1 = 111.1;
        const double value2 = 222.2;

        // Parse parameters from input JSON
        score::json::JsonParser parser;
        auto any_res = parser.FromBuffer(input);
        if (!any_res)
        {
            std::cout << "[DEBUG] JSON parsing error: " << any_res.error().Message() << std::endl;
            throw any_res.error();
        }

        const auto &obj = any_res.value().As<score::json::Object>().value().get();

        auto it1 = obj.find("kvs_parameters_1");
        auto it2 = obj.find("kvs_parameters_2");
        if (it1 == obj.end() || it2 == obj.end())
        {
            std::cout << "[DEBUG] FINDING Error: " << any_res.error().Message() << std::endl;
            throw std::runtime_error("Missing kvs_parameters_1 or kvs_parameters_2");
        }

        KvsParameters params1;
        try
        {
            auto param_json_str = extract_kvs_param_json(it1->second);
            params1 = map_to_params(param_json_str);
        }
        catch (const std::exception &e)
        {

            throw e;
        }

        const KvsParameters params2 = map_to_params(extract_kvs_param_json(it2->second));

        // First run: set values and flush

        auto kvs1 = kvs_instance(params1);
        auto kvs2 = kvs_instance(params2);

        auto set_res1 = kvs1.set_value(keyname, KvsValue(value1));
        if (!set_res1)
        {
            throw set_res1.error();
        }

        auto set_res2 = kvs2.set_value(keyname, KvsValue(value2));
        if (!set_res2)
        {
            throw set_res2.error();
        }

        kvs1.flush();
        kvs2.flush();

        // Second run: get values and log

        auto kvs1b = kvs_instance(params1);
        auto kvs2b = kvs_instance(params2);

        const auto value1_res = kvs1b.get_value(keyname);
        const auto value2_res = kvs2b.get_value(keyname);

        if (!value1_res || !value2_res)
            throw std::runtime_error("Failed to retrieve values from KVS instances");
        // Read and log using value_is_default and current_value
        const bool value_is_default1 = kvs1b.has_default_value(keyname).value();
        const double current_value1 = std::get<double>(value1_res.value().getValue());
        info_log("kvs1", keyname, current_value1);

        const bool value_is_default2 = kvs2b.has_default_value(keyname).value();
        const double current_value2 = std::get<double>(value2_res.value().getValue());
        info_log("kvs2", keyname, current_value2);
    }

    std::string SameInstanceIdSameValue::name() const { return "same_instance_id_same_value"; }
    void SameInstanceIdSameValue::run(const std::string &input) const
    {
        const std::string keyname = "number";
        const double value = 111.1;

        const KvsParameters params = map_to_params(input);

        // First run: set values and flush
        auto kvs1 = kvs_instance(params);
        auto kvs2 = kvs_instance(params);

        auto set_res1 = kvs1.set_value(keyname, KvsValue(value));
        if (!set_res1)
            throw set_res1.error();
        auto set_res2 = kvs2.set_value(keyname, KvsValue(value));
        if (!set_res2)
            throw set_res2.error();

        kvs1.flush();
        kvs2.flush();

        // Second run: get values and log
        auto kvs1b = kvs_instance(params);
        auto kvs2b = kvs_instance(params);

        const auto value1_res = kvs1b.get_value(keyname);
        const auto value2_res = kvs2b.get_value(keyname);
        if (!value1_res || !value2_res)
        {
            throw std::runtime_error("Failed to retrieve values from KVS instances");
        }

        double raw_value1 = safe_extract_double(value1_res.value().getValue());
        double raw_value2 = safe_extract_double(value2_res.value().getValue());
        info_log("kvs1", keyname, raw_value1);
        info_log("kvs2", keyname, raw_value2);
    }

    std::string SameInstanceIdDifferentValue::name() const { return "same_instance_id_diff_value"; }
    void SameInstanceIdDifferentValue::run(const std::string &input) const
    {
        const std::string keyname = "number";
        const double value1 = 111.1;
        const double value2 = 222.2;

        const KvsParameters params = map_to_params(input);

        // First run: set different values and flush
        auto kvs1 = kvs_instance(params);
        auto kvs2 = kvs_instance(params);

        auto set_res1 = kvs1.set_value(keyname, KvsValue(value1));
        if (!set_res1)
            throw set_res1.error();
        auto set_res2 = kvs2.set_value(keyname, KvsValue(value2));
        if (!set_res2)

            throw set_res2.error();

        kvs1.flush();
        kvs2.flush();

        // Second run: get values and log
        auto kvs1b = kvs_instance(params);
        auto kvs2b = kvs_instance(params);

        const auto value1_res = kvs1b.get_value(keyname);
        const auto value2_res = kvs2b.get_value(keyname);
        if (!value1_res || !value2_res)
        {
            throw std::runtime_error("Failed to retrieve values from KVS instances");
        }

        double raw_value1 = safe_extract_double(value1_res.value().getValue());
        double raw_value2 = safe_extract_double(value2_res.value().getValue());
        info_log("kvs1", keyname, raw_value1);
        info_log("kvs2", keyname, raw_value2);
    }

    ScenarioGroup::Ptr create_multiple_kvs_group()
    {
        return ScenarioGroup::Ptr{
            new ScenarioGroupImpl{"multiple_kvs",
                                  {
                                      std::make_shared<MultipleInstanceIds>(),
                                      std::make_shared<SameInstanceIdSameValue>(),
                                      std::make_shared<SameInstanceIdDifferentValue>(),
                                  },
                                  {}}};
    }
}

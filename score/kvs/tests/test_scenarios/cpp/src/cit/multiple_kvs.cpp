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

#include "multiple_kvs.hpp"
#include "helpers/kvs_instance.hpp"
#include "helpers/kvs_parameters.hpp"
#include "tracing.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace score::mw::per::kvs;
using namespace score::json;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::multiple_kvs"};

static void info_log(const std::string& instance, const std::string& keyname, double value)
{
    TRACING_INFO(kTargetName,
                 std::pair{std::string{"instance"}, instance},
                 std::pair{std::string{"key"}, keyname},
                 std::pair{std::string{"value"}, value});
}
}  // namespace

class MultipleInstanceIds : public Scenario
{
  public:
    ~MultipleInstanceIds() final = default;

    std::string name() const final
    {
        return "multiple_instance_ids";
    }

    void run(const std::string& input) const final
    {
        // Values.
        const std::string keyname{"number"};
        const double value1{111.1};
        const double value2{222.2};

        // Parameters.
        JsonParser parser;
        auto any_res{parser.FromBuffer(input)};
        if (!any_res)
        {
            throw any_res.error();
        }
        auto& obj{any_res.value().As<Object>().value().get()};

        auto params1{KvsParameters::from_object(obj.at("kvs_parameters_1").As<Object>().value().get())};
        auto params2{KvsParameters::from_object(obj.at("kvs_parameters_2").As<Object>().value().get())};
        {
            // Create first KVS instance.
            auto kvs1{kvs_instance(params1)};

            // Create second KVS instance.
            auto kvs2{kvs_instance(params2)};

            // Set value to both KVS instances.
            auto set_result_1{kvs1.set_value(keyname, KvsValue{value1})};
            if (!set_result_1)
            {
                throw std::runtime_error{"Failed to set value"};
            }
            auto set_result_2{kvs2.set_value(keyname, KvsValue{value2})};
            if (!set_result_2)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result_1{kvs1.flush()};
            if (!flush_result_1)
            {
                throw std::runtime_error{"Failed to flush first instance"};
            }
            auto flush_result_2{kvs2.flush()};
            if (!flush_result_2)
            {
                throw std::runtime_error{"Failed to flush second instance"};
            }
        }

        {
            // Second KVS run.
            auto kvs1{kvs_instance(params1)};
            auto kvs2{kvs_instance(params2)};

            auto value1{kvs1.get_value(keyname)};
            if (!value1)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            info_log("kvs1", keyname, std::get<double>(value1->getValue()));

            auto value2{kvs2.get_value(keyname)};
            if (!value2)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            info_log("kvs2", keyname, std::get<double>(value2->getValue()));
        }
    }
};

class SameInstanceIdSameValue : public Scenario
{
  public:
    ~SameInstanceIdSameValue() final = default;

    std::string name() const final
    {
        return "same_instance_id_same_value";
    }

    void run(const std::string& input) const final
    {
        // Values.
        const std::string keyname{"number"};
        const double value{111.1};

        // Parameters.
        auto params{KvsParameters::from_json(input)};
        {
            // Create first KVS instance.
            auto kvs1{kvs_instance(params)};

            // Create second KVS instance.
            auto kvs2{kvs_instance(params)};

            // Set value to both KVS instances.
            auto set_result_1{kvs1.set_value(keyname, KvsValue{value})};
            if (!set_result_1)
            {
                throw std::runtime_error{"Failed to set value"};
            }
            auto set_result_2{kvs2.set_value(keyname, KvsValue{value})};
            if (!set_result_2)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result_1{kvs1.flush()};
            if (!flush_result_1)
            {
                throw std::runtime_error{"Failed to flush first instance"};
            }
            auto flush_result_2{kvs2.flush()};
            if (!flush_result_2)
            {
                throw std::runtime_error{"Failed to flush second instance"};
            }
        }

        {
            // Second KVS run.
            auto kvs1{kvs_instance(params)};
            auto kvs2{kvs_instance(params)};

            auto value1{kvs1.get_value(keyname)};
            if (!value1)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            info_log("kvs1", keyname, std::get<double>(value1->getValue()));

            auto value2{kvs2.get_value(keyname)};
            if (!value2)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            info_log("kvs2", keyname, std::get<double>(value2->getValue()));
        }
    }
};

class SameInstanceIdDifferentValue : public Scenario
{
  public:
    ~SameInstanceIdDifferentValue() final = default;

    std::string name() const final
    {
        return "same_instance_id_diff_value";
    }

    void run(const std::string& input) const final
    {
        // Values.
        const std::string keyname{"number"};
        const double value1{111.1};
        const double value2{222.2};

        // Parameters.
        auto params{KvsParameters::from_json(input)};
        {
            // Create first KVS instance.
            auto kvs1{kvs_instance(params)};

            // Create second KVS instance.
            auto kvs2{kvs_instance(params)};

            // Set value to both KVS instances.
            auto set_result_1{kvs1.set_value(keyname, KvsValue{value1})};
            if (!set_result_1)
            {
                throw std::runtime_error{"Failed to set value"};
            }
            auto set_result_2{kvs2.set_value(keyname, KvsValue{value2})};
            if (!set_result_2)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result_1{kvs1.flush()};
            if (!flush_result_1)
            {
                throw std::runtime_error{"Failed to flush first instance"};
            }
            auto flush_result_2{kvs2.flush()};
            if (!flush_result_2)
            {
                throw std::runtime_error{"Failed to flush second instance"};
            }
        }

        {
            // Second KVS run.
            auto kvs1{kvs_instance(params)};
            auto kvs2{kvs_instance(params)};

            auto value1{kvs1.get_value(keyname)};
            if (!value1)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            info_log("kvs1", keyname, std::get<double>(value1->getValue()));

            auto value2{kvs2.get_value(keyname)};
            if (!value2)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            info_log("kvs2", keyname, std::get<double>(value2->getValue()));
        }
    }
};

ScenarioGroup::Ptr multiple_kvs_group()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"multiple_kvs",
                                                    {
                                                        std::make_shared<MultipleInstanceIds>(),
                                                        std::make_shared<SameInstanceIdSameValue>(),
                                                        std::make_shared<SameInstanceIdDifferentValue>(),
                                                    },
                                                    {}}};
}

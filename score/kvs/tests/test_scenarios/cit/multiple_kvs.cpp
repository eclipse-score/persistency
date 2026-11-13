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

#include "cit/multiple_kvs.h"

#include "helpers/kvs_instance.h"
#include "helpers/kvs_parameters.h"
#include "score/kvs/kvs_builder.h"
#include <tracing.hpp>
#include <cmath>

using namespace score::kvs;
using namespace score::json;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::multiple_kvs"};

void InfoLog(const std::string& instance, const std::string& keyname, double value)
{
    TRACING_INFO(kTargetName,
                 std::pair{std::string{"instance"}, instance},
                 std::pair{std::string{"key"}, keyname},
                 std::pair{std::string{"value"}, value});
}
}  // namespace

class MultipleInstanceIds final : public Scenario
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
        auto any_result{parser.FromBuffer(input)};
        if (!any_result)
        {
            throw score::result::Error(any_result.error());
        }
        auto& obj{any_result.value().As<Object>().value().get()};

        auto params1{KvsParameters::FromValue(obj.at("kvs_parameters_1").As<Object>().value().get())};
        auto params2{KvsParameters::FromValue(obj.at("kvs_parameters_2").As<Object>().value().get())};
        {
            // Create first KVS instance.
            auto kvs1_handle{KvsInstance(params1)};
            auto& kvs1{*kvs1_handle};

            // Create second KVS instance.
            auto kvs2_handle{KvsInstance(params2)};
            auto& kvs2{*kvs2_handle};

            // Set value to both KVS instances.
            auto set_result_1{kvs1.SetValue(keyname, KvsValue{value1})};
            if (!set_result_1)
            {
                throw std::runtime_error{"Failed to set value"};
            }
            auto set_result_2{kvs2.SetValue(keyname, KvsValue{value2})};
            if (!set_result_2)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result_1{kvs1.Flush()};
            if (!flush_result_1)
            {
                throw std::runtime_error{"Failed to flush first instance"};
            }
            auto flush_result_2{kvs2.Flush()};
            if (!flush_result_2)
            {
                throw std::runtime_error{"Failed to flush second instance"};
            }

            auto unregister_result_1{Unregister(params1.instance_id)};
            if (!unregister_result_1)
            {
                throw score::result::Error(unregister_result_1.error());
            }
            auto unregister_result_2{Unregister(params2.instance_id)};
            if (!unregister_result_2)
            {
                throw score::result::Error(unregister_result_2.error());
            }
        }

        // Reopen the instances to read the flushed files.
        {
            // Second KVS run.
            auto kvs1_handle{KvsInstance(params1)};
            auto& kvs1{*kvs1_handle};
            auto kvs2_handle{KvsInstance(params2)};
            auto& kvs2{*kvs2_handle};

            auto kvs1_value{kvs1.GetValue(keyname)};
            if (!kvs1_value)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            InfoLog("kvs1", keyname, std::get<double>(kvs1_value->GetValue()));

            auto kvs2_value{kvs2.GetValue(keyname)};
            if (!kvs2_value)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            InfoLog("kvs2", keyname, std::get<double>(kvs2_value->GetValue()));
        }
    }
};

class SameInstanceIdSameValue final : public Scenario
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
        auto params{KvsParameters::FromJson(input)};
        {
            // Create first KVS instance.
            auto kvs1_handle{KvsInstance(params)};
            auto& kvs1{*kvs1_handle};

            // Create second KVS instance.
            auto kvs2_handle{KvsInstance(params)};
            auto& kvs2{*kvs2_handle};

            // Set value to both KVS instances.
            auto set_result_1{kvs1.SetValue(keyname, KvsValue{value})};
            if (!set_result_1)
            {
                throw std::runtime_error{"Failed to set value"};
            }
            auto set_result_2{kvs2.SetValue(keyname, KvsValue{value})};
            if (!set_result_2)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result_1{kvs1.Flush()};
            if (!flush_result_1)
            {
                throw std::runtime_error{"Failed to flush first instance"};
            }
            auto flush_result_2{kvs2.Flush()};
            if (!flush_result_2)
            {
                throw std::runtime_error{"Failed to flush second instance"};
            }

            auto unregister_result{Unregister(params.instance_id)};
            if (!unregister_result)
            {
                throw score::result::Error(unregister_result.error());
            }
        }

        // Reopen the instance to read the flushed files.
        {
            // Second KVS run.
            auto kvs1_handle{KvsInstance(params)};
            auto& kvs1{*kvs1_handle};
            auto kvs2_handle{KvsInstance(params)};
            auto& kvs2{*kvs2_handle};

            auto value1{kvs1.GetValue(keyname)};
            if (!value1)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            InfoLog("kvs1", keyname, std::get<double>(value1->GetValue()));

            auto value2{kvs2.GetValue(keyname)};
            if (!value2)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            InfoLog("kvs2", keyname, std::get<double>(value2->GetValue()));
        }
    }
};

class SameInstanceIdDifferentValue final : public Scenario
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
        auto params{KvsParameters::FromJson(input)};
        {
            // Create first KVS instance.
            auto kvs1_handle{KvsInstance(params)};
            auto& kvs1{*kvs1_handle};

            // Create second KVS instance.
            auto kvs2_handle{KvsInstance(params)};
            auto& kvs2{*kvs2_handle};

            // Set value to both KVS instances.
            auto set_result_1{kvs1.SetValue(keyname, KvsValue{value1})};
            if (!set_result_1)
            {
                throw std::runtime_error{"Failed to set value"};
            }
            auto set_result_2{kvs2.SetValue(keyname, KvsValue{value2})};
            if (!set_result_2)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Both handles share one in-memory state, so the first sees the second's write before any flush.
            auto shared_value{kvs1.GetValue(keyname)};
            if (!shared_value)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            InfoLog("kvs1_before_flush", keyname, std::get<double>(shared_value->GetValue()));

            // Flush KVS.
            auto flush_result_1{kvs1.Flush()};
            if (!flush_result_1)
            {
                throw std::runtime_error{"Failed to flush first instance"};
            }
            auto flush_result_2{kvs2.Flush()};
            if (!flush_result_2)
            {
                throw std::runtime_error{"Failed to flush second instance"};
            }

            auto unregister_result{Unregister(params.instance_id)};
            if (!unregister_result)
            {
                throw score::result::Error(unregister_result.error());
            }
        }

        // Reopen the instance to read the flushed files.
        {
            // Second KVS run.
            auto kvs1_handle{KvsInstance(params)};
            auto& kvs1{*kvs1_handle};
            auto kvs2_handle{KvsInstance(params)};
            auto& kvs2{*kvs2_handle};

            auto kvs1_value{kvs1.GetValue(keyname)};
            if (!kvs1_value)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            InfoLog("kvs1", keyname, std::get<double>(kvs1_value->GetValue()));

            auto kvs2_value{kvs2.GetValue(keyname)};
            if (!kvs2_value)
            {
                throw std::runtime_error{"Failed to read value"};
            }
            InfoLog("kvs2", keyname, std::get<double>(kvs2_value->GetValue()));
        }
    }
};

ScenarioGroup::Ptr MultipleKvsGroup()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"multiple_kvs",
                                                    {
                                                        std::make_shared<MultipleInstanceIds>(),
                                                        std::make_shared<SameInstanceIdSameValue>(),
                                                        std::make_shared<SameInstanceIdDifferentValue>(),
                                                    },
                                                    {}}};
}

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

#include "cit/snapshots.h"

#include "helpers/helpers.h"
#include "helpers/kvs_instance.h"
#include "helpers/kvs_parameters.h"
#include <tracing.hpp>

using namespace score::mw::per::kvs;
using namespace score::json;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::snapshots::count"};

template <typename T>
T GetField(const Object& obj, const std::string& field)
{
    auto it{obj.find(field)};
    if (it == obj.end())
    {
        throw std::runtime_error("Missing field: " + field);
    }
    return it->second.As<T>().value();
}

Object GetObject(const std::string& data)
{
    JsonParser parser;
    auto from_buffer_result{parser.FromBuffer(data)};
    if (!from_buffer_result)
    {
        throw std::runtime_error{"Failed to parse JSON"};
    }

    auto as_object_result{from_buffer_result.value().As<Object>()};
    if (!as_object_result)
    {
        throw std::runtime_error{"Failed to cast JSON to object"};
    }

    return std::move(as_object_result.value().get());
}
}  // namespace

class SnapshotCount final : public Scenario
{
  public:
    ~SnapshotCount() final = default;

    std::string name() const final
    {
        return "count";
    }

    void run(const std::string& input) const final
    {
        auto obj{GetObject(input)};
        auto count{GetField<int32_t>(obj, "count")};
        auto params{KvsParameters::FromJson(input)};

        // Create snapshots.
        for (int32_t i{0}; i < count; ++i)
        {
            auto kvs{KvsInstance(params)};
            auto set_result{kvs.SetValue("counter", KvsValue{i})};
            if (!set_result)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            auto count_result{kvs.SnapshotCount()};
            if (!count_result)
            {
                throw std::runtime_error{"Unable to get snapshot count"};
            }

            TRACING_INFO(kTargetName, std::pair{std::string{"snapshot_count"}, count_result.value()});

            // Flush KVS.
            auto flush_result{kvs.Flush()};
            if (!flush_result)
            {
                throw std::runtime_error{"Failed to flush"};
            }
        }

        {
            auto kvs{KvsInstance(params)};
            auto count_result{kvs.SnapshotCount()};
            if (!count_result)
            {
                throw std::runtime_error{"Unable to get snapshot count"};
            }
            TRACING_INFO(kTargetName, std::pair{std::string{"snapshot_count"}, count_result.value()});
        }
    }
};

class SnapshotMaxCount final : public Scenario
{
  public:
    ~SnapshotMaxCount() final = default;

    std::string name() const final
    {
        return "max_count";
    }

    void run(const std::string& input) const final
    {
        auto params{KvsParameters::FromJson(input)};

        auto kvs{KvsInstance(params)};
        TRACING_INFO(kTargetName, std::pair{std::string{"max_count"}, kvs.SnapshotMaxCount()});
    }
};

class SnapshotRestore final : public Scenario
{
  public:
    ~SnapshotRestore() final = default;

    std::string name() const final
    {
        return "restore";
    }

    void run(const std::string& input) const final
    {
        auto obj{GetObject(input)};
        auto count{GetField<int32_t>(obj, "count")};
        auto snapshot_id{GetField<uint64_t>(obj, "snapshot_id")};
        auto params{KvsParameters::FromJson(input)};

        // Create snapshots.
        for (int32_t i{0}; i < count; ++i)
        {
            auto kvs{KvsInstance(params)};
            auto set_result{kvs.SetValue("counter", KvsValue{i})};
            if (!set_result)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result{kvs.Flush()};
            if (!flush_result)
            {
                throw std::runtime_error{"Failed to flush"};
            }
        }

        {
            auto kvs{KvsInstance(params)};

            auto restore_result{kvs.SnapshotRestore(snapshot_id)};
            TRACING_INFO(kTargetName,
                         std::pair{std::string{"result"}, restore_result ? "Ok(())" : "Err(InvalidSnapshotId)"});

            if (restore_result)
            {
                auto get_result{kvs.GetValue("counter")};
                if (!get_result)
                {
                    throw std::runtime_error{"Failed to read value"};
                }

                auto value{std::get<int32_t>(get_result.value().GetValue())};
                TRACING_INFO(kTargetName, std::pair{std::string{"value"}, value});
            }
        }
    }
};

class SnapshotPaths final : public Scenario
{
  public:
    ~SnapshotPaths() final = default;

    std::string name() const final
    {
        return "paths";
    }

    void run(const std::string& input) const final
    {
        auto obj{GetObject(input)};
        auto count{GetField<int32_t>(obj, "count")};
        auto snapshot_id{GetField<uint64_t>(obj, "snapshot_id")};
        auto params{KvsParameters::FromJson(input)};
        auto working_dir{*params.dir};
        auto instance_id{params.instance_id};

        // Create snapshots.
        for (int32_t i{0}; i < count; ++i)
        {
            auto kvs{KvsInstance(params)};
            auto set_result{kvs.SetValue("counter", KvsValue{i})};
            if (!set_result)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result{kvs.Flush()};
            if (!flush_result)
            {
                throw std::runtime_error{"Failed to flush"};
            }
        }

        {
            auto [kvs_path, hash_path] = KvsHashPaths(working_dir, instance_id, snapshot_id);
            TRACING_INFO(kTargetName,
                         std::make_pair(std::string{"kvs_path"}, kvs_path),
                         std::make_pair(std::string{"hash_path"}, hash_path));
        }
    }
};

ScenarioGroup::Ptr SnapshotsGroup()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"snapshots",
                                                    {std::make_shared<SnapshotCount>(),
                                                     std::make_shared<SnapshotMaxCount>(),
                                                     std::make_shared<SnapshotRestore>(),
                                                     std::make_shared<SnapshotPaths>()},
                                                    {}}};
}

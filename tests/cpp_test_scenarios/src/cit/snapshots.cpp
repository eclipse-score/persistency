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

#include "snapshots.hpp"
#include "helpers/helpers.hpp"
#include "helpers/kvs_instance.hpp"
#include "helpers/kvs_parameters.hpp"
#include "tracing.hpp"

using namespace score::mw::per::kvs;
using namespace score::json;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::snapshots::count"};

template <typename T>
T get_field(const Object& obj, const std::string& field)
{
    auto it{obj.find(field)};
    if (it == obj.end())
    {
        throw std::runtime_error("Missing field: " + field);
    }
    return it->second.As<T>().value();
}

Object get_object(const std::string& data)
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

class SnapshotCount : public Scenario
{
  public:
    ~SnapshotCount() final = default;

    std::string name() const final
    {
        return "count";
    }

    /**
     * Requirement not being met:
     *   - The snapshot is created for each data stored.
     *   - Max count should be configurable.
     *
     * TestSnapshotCountFirstFlush
     *      Issue: The test expects the final snapshot_count to be min(count,
     * snapshot_max_count) (e.g., 1 for count=1, snapshot_max_count=1/3/10).
     *      Observed: C++ emits snapshot_count: 0 after the first flush.
     *      Possible Root Cause: In C++, the snapshot count is not incremented after
     * the first flush because the snapshot rotation logic and counting are tied to
     * the hardcoded max (not the parameter).
     *
     * TestSnapshotCountFull
     *      Issue: The test expects a sequence of snapshot_count values: [0, 1] for count=2, [0, 1,
     * 2, 3] for count=4, etc. Observed: C++ emits [0, 0, 1] or [0, 0, 1, 2, 3], but the first value
     * is always 0, and the final value is not as expected. Possible Root Cause: The C++
     * implementation may not be accumulating the count correctly, it stores or updates the count
     * only after flush when MAX<3.
     *
     * Raised bugs: https://github.com/eclipse-score/persistency/issues/108
     *              https://github.com/eclipse-score/persistency/issues/192
     */
    void run(const std::string& input) const final
    {
        auto obj{get_object(input)};
        auto count{get_field<int32_t>(obj, "count")};
        auto params{KvsParameters::from_json(input)};

        // Create snapshots.
        for (int32_t i{0}; i < count; ++i)
        {
            auto kvs{kvs_instance(params)};
            auto set_result{kvs.set_value("counter", KvsValue{i})};
            if (!set_result)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            auto count_result{kvs.snapshot_count()};
            if (!count_result)
            {
                throw std::runtime_error{"Unable to get snapshot count"};
            }

            TRACING_INFO(kTargetName, std::pair{std::string{"snapshot_count"}, count_result.value()});

            // Flush KVS.
            auto flush_result{kvs.flush()};
            if (!flush_result)
            {
                throw std::runtime_error{"Failed to flush"};
            }
        }

        {
            auto kvs{kvs_instance(params)};
            auto count_result{kvs.snapshot_count()};
            if (!count_result)
            {
                throw std::runtime_error{"Unable to get snapshot count"};
            }
            TRACING_INFO(kTargetName, std::pair{std::string{"snapshot_count"}, count_result.value()});
        }
    }
};

class SnapshotMaxCount : public Scenario
{
  public:
    ~SnapshotMaxCount() final = default;

    std::string name() const final
    {
        return "max_count";
    }

    void run(const std::string& input) const final
    {
        auto obj{get_object(input)};
        auto count{get_field<int32_t>(obj, "count")};
        auto params{KvsParameters::from_json(input)};

        auto kvs{kvs_instance(params)};
        TRACING_INFO(kTargetName, std::pair{std::string{"max_count"}, kvs.snapshot_max_count()});
    }
};

class SnapshotRestore : public Scenario
{
  public:
    ~SnapshotRestore() final = default;

    std::string name() const final
    {
        return "restore";
    }

    void run(const std::string& input) const final
    {
        auto obj{get_object(input)};
        auto count{get_field<int32_t>(obj, "count")};
        auto snapshot_id{get_field<uint64_t>(obj, "snapshot_id")};
        auto params{KvsParameters::from_json(input)};

        // Create snapshots.
        for (int32_t i{0}; i < count; ++i)
        {
            auto kvs{kvs_instance(params)};
            auto set_result{kvs.set_value("counter", KvsValue{i})};
            if (!set_result)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result{kvs.flush()};
            if (!flush_result)
            {
                throw std::runtime_error{"Failed to flush"};
            }
        }

        {
            auto kvs{kvs_instance(params)};

            auto restore_result{kvs.snapshot_restore(snapshot_id)};
            TRACING_INFO(kTargetName,
                         std::pair{std::string{"result"}, restore_result ? "Ok(())" : "Err(InvalidSnapshotId)"});

            if (restore_result)
            {
                auto get_result{kvs.get_value("counter")};
                if (!get_result)
                {
                    throw std::runtime_error{"Failed to read value"};
                }

                auto value{std::get<int32_t>(get_result.value().getValue())};
                TRACING_INFO(kTargetName, std::pair{std::string{"value"}, value});
            }
        }
    }
};

class SnapshotPaths : public Scenario
{
  public:
    ~SnapshotPaths() final = default;

    std::string name() const final
    {
        return "paths";
    }

    void run(const std::string& input) const final
    {
        auto obj{get_object(input)};
        auto count{get_field<int32_t>(obj, "count")};
        auto snapshot_id{get_field<uint64_t>(obj, "snapshot_id")};
        auto params{KvsParameters::from_json(input)};
        auto working_dir{*params.dir};
        auto instance_id{params.instance_id};

        // Create snapshots.
        for (int32_t i{0}; i < count; ++i)
        {
            auto kvs{kvs_instance(params)};
            auto set_result{kvs.set_value("counter", KvsValue{i})};
            if (!set_result)
            {
                throw std::runtime_error{"Failed to set value"};
            }

            // Flush KVS.
            auto flush_result{kvs.flush()};
            if (!flush_result)
            {
                throw std::runtime_error{"Failed to flush"};
            }
        }

        {
            auto [kvs_path, hash_path] = kvs_hash_paths(working_dir, instance_id, snapshot_id);
            TRACING_INFO(kTargetName,
                         std::make_pair(std::string{"kvs_path"}, kvs_path),
                         std::make_pair(std::string{"hash_path"}, hash_path));
        }
    }
};

ScenarioGroup::Ptr snapshots_group()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"snapshots",
                                                    {std::make_shared<SnapshotCount>(),
                                                     std::make_shared<SnapshotMaxCount>(),
                                                     std::make_shared<SnapshotRestore>(),
                                                     std::make_shared<SnapshotPaths>()},
                                                    {}}};
}

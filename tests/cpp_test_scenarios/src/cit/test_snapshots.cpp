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

#include "test_snapshots.hpp"
#include "helpers/kvs_instance.hpp"
#include "helpers/kvs_parameters.hpp"
#include "tracing.hpp"

namespace test_snapshots
{

    const std::string kTargetName{"cpp_test_scenarios::snapshots::count"};

    // Helper to extract required int64_t field from JSON object
    template <typename T>
    T extract_json_field(const std::map<score::memory::StringComparisonAdaptor,
                                        score::json::Any> &obj,
                         const std::string &field)
    {
        auto it = obj.find(field);
        if (it == obj.end())
        {
            throw std::runtime_error("Missing field: " + field);
        }
        return static_cast<T>(it->second.As<int64_t>().value());
    }

    uint8_t get_count(const std::string &data)
    {
        using namespace score::json;
        JsonParser parser;
        auto any_res{parser.FromBuffer(data)};
        if (!any_res)
        {
            throw any_res.error();
        }
        const auto &obj = any_res.value().As<Object>().value().get();
        return extract_json_field<uint8_t>(obj, "count");
    }

    std::string SnapshotCount::name() const { return "count"; }
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
     *      Issue: The test expects a sequence of snapshot_count values: [0, 1] for count=2, [0, 1, 2, 3]
     * for count=4, etc.
     *      Observed: C++ emits [0, 0, 1] or [0, 0, 1, 2, 3], but the
     * first value is always 0, and the final value is not as expected.
     *      Possible Root Cause: The C++ implementation may not be accumulating the count
     * correctly, it stores or updates the count only after flush when MAX<3.
     *
     * Raised bugs: https://github.com/eclipse-score/persistency/issues/108
     *              https://github.com/eclipse-score/persistency/issues/192
     */
    void SnapshotCount::run(const std::string &input) const
    {
        using namespace score::mw::per::kvs;
        auto count = get_count(input);
        KvsParameters params = map_to_params(input);
        auto kvs = kvs_instance(params);
        for (uint8_t i = 0; i < count; ++i)
        {
            auto set_res =
                kvs.set_value("counter", KvsValue(static_cast<int64_t>(i)));
            if (!set_res)
            {
                throw set_res.error();
            }
            auto snap_res = kvs.snapshot_count();
            if (!snap_res)
            {
                throw snap_res.error();
            }
            TRACING_INFO(
                kTargetName,
                std::pair{std::string{"snapshot_count"}, static_cast<int>(snap_res.value())});

            auto flush_res = kvs.flush();
            if (!flush_res)
            {
                throw flush_res.error();
            }
        }
        {
            auto snap_res = kvs.snapshot_count();
            if (!snap_res)
            {
                throw snap_res.error();
            }
            TRACING_INFO(
                kTargetName,
                std::pair{std::string{"snapshot_count"}, static_cast<int>(snap_res.value())});
        }
    }

    std::string SnapshotMaxCount::name() const { return "max_count"; }
    void SnapshotMaxCount::run(const std::string &input) const
    {
        using namespace score::mw::per::kvs;
        KvsParameters params = map_to_params(input);
        auto kvs = kvs_instance(params);
        TRACING_INFO(kTargetName,
                     std::pair{std::string{"max_count"}, kvs.snapshot_max_count()});
    }

    std::string SnapshotRestore::name() const { return "restore"; }
    void SnapshotRestore::run(const std::string &input) const
    {
        using namespace score::mw::per::kvs;
        using namespace score::json;
        KvsParameters params = map_to_params(input);
        JsonParser parser;
        auto any_res{parser.FromBuffer(input)};
        if (!any_res)
        {
            throw any_res.error();
        }
        const auto &obj = any_res.value().As<Object>().value().get();
        uint8_t count = extract_json_field<uint8_t>(obj, "count");
        int snapshot_id = extract_json_field<int>(obj, "snapshot_id");
        auto kvs = kvs_instance(params);
        for (uint8_t i = 0; i < count; ++i)
        {
            auto set_res = kvs.set_value("counter", KvsValue(static_cast<int64_t>(i)));
            if (!set_res)
            {
                throw set_res.error();
            }
            auto flush_res = kvs.flush();
            if (!flush_res)
            {
                throw flush_res.error();
            }
        }
        {
            const auto restore_res = kvs.snapshot_restore(snapshot_id);
            TRACING_INFO(
                kTargetName,
                std::pair{std::string{"result"},
                          restore_res ? "Ok(())" : "Err(InvalidSnapshotId)"});
            if (restore_res)
            {
                const auto value_res = kvs.get_value("counter");
                if (value_res)
                {
                    TRACING_INFO(kTargetName,
                                 std::pair{std::string{"value"},
                                           static_cast<int>(std::get<int64_t>(
                                               value_res.value().getValue()))});
                }
            }
        }
    }

    std::string SnapshotPaths::name() const { return "paths"; }
    void SnapshotPaths::run(const std::string &input) const
    {
        using namespace score::mw::per::kvs;
        using namespace score::json;
        KvsParameters params = map_to_params(input);
        JsonParser parser;
        auto any_res{parser.FromBuffer(input)};
        if (!any_res)
        {
            throw any_res.error();
        }
        const auto &obj = any_res.value().As<Object>().value().get();
        uint8_t count = extract_json_field<uint8_t>(obj, "count");
        int snapshot_id = extract_json_field<int>(obj, "snapshot_id");
        auto kvs = kvs_instance(params);
        for (uint8_t i = 0; i < count; ++i)
        {
            auto set_res = kvs.set_value("counter", KvsValue(static_cast<int64_t>(i)));
            if (!set_res)
            {
                throw set_res.error();
            }
            auto flush_res = kvs.flush();
            if (!flush_res)
            {
                throw flush_res.error();
            }
        }
        {
            const auto kvs_path_res = kvs.get_kvs_filename(snapshot_id);
            const auto hash_path_res = kvs.get_hash_filename(snapshot_id);
            TRACING_INFO(
                kTargetName,
                std::make_pair(
                    std::string("kvs_path"),
                    kvs_path_res
                        ? ("Ok(\"" + std::string(kvs_path_res.value()) + "\")")
                        : "Err(FileNotFound)"),
                std::make_pair(
                    std::string("hash_path"),
                    hash_path_res
                        ? ("Ok(\"" + std::string(hash_path_res.value()) + "\")")
                        : "Err(FileNotFound)"));
        }
    }

    ScenarioGroup::Ptr create_snapshots_group()
    {
        return ScenarioGroup::Ptr{
            new ScenarioGroupImpl{"snapshots",
                                  {std::make_shared<SnapshotCount>(),
                                   std::make_shared<SnapshotMaxCount>(),
                                   std::make_shared<SnapshotRestore>(),
                                   std::make_shared<SnapshotPaths>()},
                                  {}}};
    }
} // namespace test_snapshots

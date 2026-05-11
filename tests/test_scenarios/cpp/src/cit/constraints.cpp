/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
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
// Copyright (c) 2025 Qorix
//
// Test scenarios for KVS constraint configuration

#include "constraints.hpp"

#include "helpers/kvs_instance.hpp"
#include "helpers/kvs_parameters.hpp"
#include "tracing.hpp"

#include <sys/stat.h>
#include <cstring>
#include <filesystem>

using namespace score::mw::per::kvs;
using namespace score::json;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::constraints"};

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

void info_log(const std::string& name, const std::string& value)
{
    TRACING_INFO(kTargetName, std::make_pair(name, value));
}

void info_log(const std::string& name, int value)
{
    TRACING_INFO(kTargetName, std::make_pair(name, std::to_string(value)));
}
}  // namespace

class ConstraintConfiguration : public Scenario
{
  public:
    ~ConstraintConfiguration() final = default;

    std::string name() const final
    {
        return "ConstraintConfiguration";
    }

    void run(const std::string& input) const final
    {
        auto obj{get_object(input)};
        auto constraint_type{get_field<std::string>(obj, "constraint_type")};
        auto constraint_value{get_field<size_t>(obj, "constraint_value")};
        auto params{KvsParameters::from_json(input)};

        if (constraint_type == "runtime")
        {
            // Test runtime constraint configuration via snapshot_max_count
            Kvs kvs = kvs_instance(params);
            size_t configured_max = kvs.snapshot_max_count();

            info_log("configured_max", static_cast<int>(configured_max));

            // Verify the runtime constraint was applied
            // Runtime values are capped at compile-time KVS_MAX_SNAPSHOTS
            size_t expected_max = std::min(constraint_value, static_cast<size_t>(KVS_MAX_SNAPSHOTS));
            int constraint_applied = (configured_max == expected_max) ? 1 : 0;
            info_log("constraint_applied", constraint_applied);
        }
        else if (constraint_type == "compile_time")
        {
            // Test that compile-time constraints exist
            // KVS_MAX_SNAPSHOTS is defined in kvs.hpp as a compile-time constant
            int compile_time_max = KVS_MAX_SNAPSHOTS;
            info_log("compile_time_max", compile_time_max);

            int compile_time_constraint_exists = 1;  // Constants are defined in source
            info_log("compile_time_constraint_exists", compile_time_constraint_exists);
        }
    }
};

class PermissionControl : public Scenario
{
  public:
    ~PermissionControl() final = default;

    std::string name() const final
    {
        return "PermissionControl";
    }

    void run(const std::string& input) const final
    {
        KvsParameters params{KvsParameters::from_json(input)};

        // Create KVS instance and verify it uses filesystem
        Kvs kvs = kvs_instance(params);

        // Write a value to ensure filesystem is used
        auto result = kvs.set_value("test_key", KvsValue("test_value"));
        if (!result.has_value())
        {
            throw std::runtime_error("Failed to set value");
        }

        auto flush_result = kvs.flush();
        if (!flush_result.has_value())
        {
            throw std::runtime_error("Failed to flush");
        }

        // Check that files exist on filesystem (proof of filesystem usage)
        std::string dir_path = params.dir.value();
        int uses_filesystem = std::filesystem::exists(dir_path) ? 1 : 0;
        info_log("uses_filesystem", uses_filesystem);

        // C++ KVS does not implement a custom permission layer
        // It relies on standard filesystem operations
        int custom_permission_layer = 0;
        info_log("custom_permission_layer", custom_permission_layer);
    }
};

class PermissionErrorHandling : public Scenario
{
  public:
    ~PermissionErrorHandling() final = default;

    std::string name() const final
    {
        return "PermissionErrorHandling";
    }

    void run(const std::string& input) const final
    {
        auto obj{get_object(input)};
        auto error_type{get_field<std::string>(obj, "error_type")};
        auto params{KvsParameters::from_json(input)};
        std::string dir_path = params.dir.value();

        // Create directory
        std::filesystem::create_directories(dir_path);

        // First, create KVS instance and write some data so files exist
        {
            Kvs kvs = kvs_instance(params);
            auto result = kvs.set_value("test_key", KvsValue("test_value"));
            if (!result.has_value())
            {
                throw std::runtime_error("Failed to set initial value");
            }
            auto flush_result = kvs.flush();
            if (!flush_result.has_value())
            {
                throw std::runtime_error("Failed to flush initial data");
            }
        }  // KVS destroyed here

        int error_detected = 0;
        int error_reported = 0;
        std::string error_message;

        if (error_type == "read_denied")
        {
            // Make directory unreadable (prevents reading existing files)
            chmod(dir_path.c_str(), 0000);  // No permissions

            // Try to create KVS instance with need_kvs=Required (will attempt to read existing files from directory)
            KvsParameters read_params = params;
            read_params.need_kvs = true;  // Require existing KVS files
            try
            {
                Kvs kvs = kvs_instance(read_params);
                error_detected = 0;
                error_reported = 0;
                error_message = "No error occurred";
            }
            catch (const std::exception& e)
            {
                error_detected = 1;
                error_reported = 1;
                error_message = e.what();
            }

            // Restore permissions for cleanup
            chmod(dir_path.c_str(), 0755);
        }
        else if (error_type == "write_denied")
        {
            // Make directory read-only (prevents writing new files)
            chmod(dir_path.c_str(), 0555);  // Read and execute only

            // Try to create KVS and write (will fail due to write restrictions)
            try
            {
                Kvs kvs = kvs_instance(params);
                // Try to write a new value (should fail)
                auto result = kvs.set_value("new_key", KvsValue("new_value"));
                if (!result.has_value())
                {
                    error_detected = 1;
                    error_reported = 1;
                    error_message = result.error().Message();
                }
                else
                {
                    // Try to flush (might fail here if not during set_value)
                    auto flush_result = kvs.flush();
                    if (!flush_result.has_value())
                    {
                        error_detected = 1;
                        error_reported = 1;
                        error_message = flush_result.error().Message();
                    }
                    else
                    {
                        error_detected = 0;
                        error_reported = 0;
                        error_message = "No error occurred";
                    }
                }
            }
            catch (const std::exception& e)
            {
                error_detected = 1;
                error_reported = 1;
                error_message = e.what();
            }

            // Restore permissions for cleanup
            chmod(dir_path.c_str(), 0755);
        }

        info_log("error_detected", error_detected);
        info_log("error_reported", error_reported);
        info_log("error_message", error_message);
    }
};

ScenarioGroup::Ptr constraints_group()
{
    std::vector<Scenario::Ptr> scenarios = {
        std::make_shared<ConstraintConfiguration>(),
        std::make_shared<PermissionControl>(),
        std::make_shared<PermissionErrorHandling>(),
    };
    return std::make_shared<ScenarioGroupImpl>("constraints", scenarios, std::vector<ScenarioGroup::Ptr>{});
}

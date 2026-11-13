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

#include "cit/persistency.h"

#include "helpers/kvs_instance.h"
#include "helpers/kvs_parameters.h"
#include <tracing.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace score::kvs;

namespace
{
const std::string kTargetName{"cpp_test_scenarios::cit::persistency"};

/// @brief Every file in the directory as "name=fingerprint", sorted and joined with ','.
///
/// The fingerprint is a hash of the file content, comparable within one run only.
std::string StorageFingerprints(const std::string& dir)
{
    std::vector<std::string> entries;
    for (const auto& entry : std::filesystem::directory_iterator{dir})
    {
        std::ifstream file{entry.path(), std::ios::binary};
        if (!file)
        {
            throw std::runtime_error{"Failed to read file: " + entry.path().string()};
        }
        const std::string content{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
        entries.push_back(entry.path().filename().string() + "=" + std::to_string(std::hash<std::string>{}(content)));
    }
    std::sort(entries.begin(), entries.end());

    std::string storage;
    for (const auto& entry : entries)
    {
        storage += storage.empty() ? entry : "," + entry;
    }
    return storage;
}

/// @brief Logs every written key of the KVS with its I32 value.
void LogStoredValues(const Kvs& kvs)
{
    const auto keys{kvs.GetAllKeys()};
    if (!keys)
    {
        throw std::runtime_error{"Failed to get keys: " + std::string{keys.error().Message()}};
    }

    for (const auto& key : keys.value())
    {
        const auto value{kvs.GetValue(key)};
        if (!value)
        {
            throw std::runtime_error{"Failed to read value of key " + key + ": " +
                                     std::string{value.error().Message()}};
        }
        const auto* const number{std::get_if<int32_t>(&value.value().GetValue())};
        if (number == nullptr)
        {
            throw std::runtime_error{"Value is not an I32, key: " + key};
        }
        TRACING_INFO(kTargetName, std::pair{std::string{"key"}, key}, std::pair{std::string{"value"}, *number});
    }
}

/// @brief Logs the error of a failed load from storage under "load_error", logs nothing on success.
void LogLoadError(const score::Result<void>& result)
{
    if (!result)
    {
        TRACING_INFO(kTargetName, std::pair{std::string{"load_error"}, std::string{result.error().Message()}});
    }
}
}  // namespace

/// @brief Changes the KVS, flushes it twice, changes it again without flush, then destroys the KVS object.
///
/// The second flush overwrites and removes keys already in storage.
/// The changes after it end with a snapshot restore, which replaces the in-memory data.
/// Logs the files in the working directory with their fingerprints after each step.
class ExplicitFlush final : public Scenario
{
  public:
    ~ExplicitFlush() final = default;

    std::string name() const final
    {
        return "explicit_flush";
    }

    void run(const std::string& input) const final
    {
        const auto params{KvsParameters::FromJson(input)};
        const auto dir{params.dir.value()};

        // Throws if the step failed, then logs the files in the working directory with their fingerprints.
        const auto step{[&dir](const std::string& name, const score::Result<void>& result) {
            if (!result)
            {
                throw std::runtime_error{"Failed to " + name + ": " + std::string{result.error().Message()}};
            }
            TRACING_INFO(kTargetName,
                         std::pair{std::string{"step"}, name},
                         std::pair{std::string{"storage"}, StorageFingerprints(dir)});
        }};

        {
            auto kvs{KvsInstance(params)};
            step("open", {});

            // Changes before the first flush, on empty storage: set.
            step("set", kvs.SetValue("kept", KvsValue{1}));
            step("set", kvs.SetValue("overwritten", KvsValue{1}));
            step("set", kvs.SetValue("removed", KvsValue{3}));
            step("flush", kvs.Flush());

            // Changes to stored keys, then the second flush: overwrite, remove.
            step("set", kvs.SetValue("overwritten", KvsValue{2}));
            step("remove", kvs.RemoveKey("removed"));
            step("flush", kvs.Flush());

            // Changes after the last flush, never flushed: overwrite, remove, reset, set, snapshot restore.
            // Snapshot 1 holds the state of the first flush.
            step("set", kvs.SetValue("overwritten", KvsValue{20}));
            step("remove", kvs.RemoveKey("kept"));
            step("reset", kvs.Reset());
            step("set", kvs.SetValue("added", KvsValue{4}));
            step("restore", kvs.SnapshotRestore(SnapshotId{1}));
        }

        // The KVS object is destroyed: a file written now was written by the destructor.
        step("drop", {});
    }
};

/// @brief Opens the KVS and logs every written key with its I32 value.
class StoredValues final : public Scenario
{
  public:
    ~StoredValues() final = default;

    std::string name() const final
    {
        return "stored_values";
    }

    void run(const std::string& input) const final
    {
        const auto params{KvsParameters::FromJson(input)};
        const auto kvs{KvsInstance(params)};
        LogStoredValues(kvs);
    }
};

/// @brief Opens the KVS, restores snapshot 1 and logs the error if the restore fails.
/// Then logs every written key with its I32 value.
class ValuesAfterRestore final : public Scenario
{
  public:
    ~ValuesAfterRestore() final = default;

    std::string name() const final
    {
        return "values_after_restore";
    }

    void run(const std::string& input) const final
    {
        const auto params{KvsParameters::FromJson(input)};
        auto kvs{KvsInstance(params)};
        LogLoadError(kvs.SnapshotRestore(SnapshotId{1}));
        LogStoredValues(kvs);
    }
};

/// @brief Opens the KVS, discards pending changes (reloads the stored data) and logs the error if this fails.
/// Then logs every written key with its I32 value.
class ValuesAfterDiscard final : public Scenario
{
  public:
    ~ValuesAfterDiscard() final = default;

    std::string name() const final
    {
        return "values_after_discard";
    }

    void run(const std::string& input) const final
    {
        const auto params{KvsParameters::FromJson(input)};
        auto kvs{KvsInstance(params)};
        LogLoadError(kvs.DiscardPendingChanges());
        LogStoredValues(kvs);
    }
};

ScenarioGroup::Ptr PersistencyGroup()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{"persistency",
                                                    {std::make_shared<ExplicitFlush>(),
                                                     std::make_shared<StoredValues>(),
                                                     std::make_shared<ValuesAfterRestore>(),
                                                     std::make_shared<ValuesAfterDiscard>()},
                                                    {}}};
}

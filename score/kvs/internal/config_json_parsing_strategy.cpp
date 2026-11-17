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
#include "score/kvs/internal/config_json_parsing_strategy.h"

#include "score/filesystem/filesystem.h"
#include "score/kvs/error.h"
#include "score/kvs/internal/logger.h"

#include <score/optional.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>
#include <utility>

namespace score::kvs
{

namespace
{

score::Unexpected Fail(const std::string_view reason) noexcept
{
    GetLogger().LogError() << "Invalid configuration:" << reason;
    return score::MakeUnexpected(ErrorCode::kInvalidConfiguration);
}

template <typename Container>
bool HasOnlyKeys(const score::json::Object& object, const Container& allowed) noexcept
{
    return std::none_of(object.begin(), object.end(), [&allowed](const auto& member) {
        const std::string_view key{member.first.GetAsStringView()};
        return std::none_of(allowed.begin(), allowed.end(), [key](const std::string_view name) {
            return key == name;
        });
    });
}

// Reads an optional integer member and checks it against a minimum.
// Returns false if the member is present but invalid.
bool ReadInteger(const score::json::Object& entry,
                 const std::string_view key,
                 const std::int64_t minimum,
                 score::cpp::optional<std::size_t>& out)
{
    const auto it{entry.find(key)};
    if (it == entry.end())
    {
        return true;
    }
    const auto value{it->second.As<std::int64_t>()};
    if (!value.has_value() || (*value < minimum))
    {
        return false;
    }
    out = static_cast<std::size_t>(*value);
    return true;
}

// Reads an optional required/optional/ignored member.
// Returns the index into kConfigLoadPolicyValues, no value if absent, or false if invalid.
bool ReadPolicy(const score::json::Object& entry, const std::string_view key, score::cpp::optional<std::size_t>& out)
{
    const auto it{entry.find(key)};
    if (it == entry.end())
    {
        return true;
    }
    const auto value{it->second.As<std::string>()};
    if (!value.has_value())
    {
        return false;
    }
    const std::string_view text{value->get()};
    const auto* const found{std::find(kConfigLoadPolicyValues.begin(), kConfigLoadPolicyValues.end(), text)};
    if (found == kConfigLoadPolicyValues.end())
    {
        return false;
    }
    out = static_cast<std::size_t>(found - kConfigLoadPolicyValues.begin());
    return true;
}

score::Result<KvsParameters> ParseEntry(const score::json::Any& element)
{
    const auto object{element.As<score::json::Object>()};
    if (!object.has_value())
    {
        return Fail("Entry is not an object");
    }
    const score::json::Object& entry{object->get()};
    if (!HasOnlyKeys(entry, kConfigEntryKeys))
    {
        return Fail("Entry has an unknown key");
    }

    score::cpp::optional<std::size_t> instance_id{};
    if (!ReadInteger(entry, "instanceId", 0, instance_id) || !instance_id.has_value())
    {
        return Fail("Entry needs a non-negative integer instanceId");
    }
    KvsParameters options{InstanceId{*instance_id}};

    score::cpp::optional<std::size_t> snapshot_id{};
    if (!ReadInteger(entry, "snapshotId", 0, snapshot_id))
    {
        return Fail("snapshotId must be a non-negative integer");
    }
    options.snapshot_id = snapshot_id.value_or(options.snapshot_id);

    score::cpp::optional<std::size_t> defaults{};
    if (!ReadPolicy(entry, "defaults", defaults))
    {
        return Fail("defaults must be required, optional or ignored");
    }
    if (defaults.has_value())
    {
        constexpr std::array<KvsDefaults, 3> kValues{
            KvsDefaults::kRequired, KvsDefaults::kOptional, KvsDefaults::kIgnored};
        options.default_load_policy = kValues.at(*defaults);
    }

    score::cpp::optional<std::size_t> kvs_load{};
    if (!ReadPolicy(entry, "kvsLoad", kvs_load))
    {
        return Fail("kvsLoad must be required, optional or ignored");
    }
    if (kvs_load.has_value())
    {
        constexpr std::array<KvsLoad, 3> kValues{KvsLoad::kRequired, KvsLoad::kOptional, KvsLoad::kIgnored};
        options.current_load_policy = kValues.at(*kvs_load);
    }

    if (const auto it{entry.find("workingDir")}; it != entry.end())
    {
        const auto value{it->second.As<std::string>()};
        if (!value.has_value())
        {
            return Fail("workingDir must be a string");
        }
        options.working_dir = value->get();
    }

    score::cpp::optional<std::size_t> max_storage_bytes{};
    if (!ReadInteger(entry, "maxStorageBytes", 1, max_storage_bytes))
    {
        return Fail("maxStorageBytes must be a positive integer");
    }
    if (max_storage_bytes.has_value())
    {
        options.storage_quota = *max_storage_bytes;
    }

    score::cpp::optional<std::size_t> snapshot_max_count{};
    if (!ReadInteger(entry, "snapshotMaxCount", 0, snapshot_max_count))
    {
        return Fail("snapshotMaxCount must be a non-negative integer");
    }
    options.snapshot_count_limit = snapshot_max_count.value_or(options.snapshot_count_limit);

    return options;
}

}  // namespace

score::Result<KvsConfiguration> ConfigJsonParsingStrategy::Parse(const std::string_view path) const noexcept
{
    const score::filesystem::Filesystem filesystem{score::filesystem::FilesystemFactory{}.CreateInstance()};
    const auto stream{filesystem.streams->Open(score::filesystem::Path{std::string{path}}, std::ios::in)};
    if (!stream.has_value())
    {
        GetLogger().LogError() << "Cannot open configuration file" << path;
        return score::MakeUnexpected(ErrorCode::kConfigFileReadError);
    }

    // istream::read() turns a failing read into badbit, while istreambuf_iterator would let the exception escape.
    std::iostream& input{**stream};
    std::string content{};
    constexpr std::size_t kReadChunkSize{4096U};
    std::array<char, kReadChunkSize> chunk{};
    while (input.good())
    {
        input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        content.append(chunk.data(), static_cast<std::size_t>(input.gcount()));
    }
    if (input.bad())
    {
        GetLogger().LogError() << "Cannot read configuration file" << path;
        return score::MakeUnexpected(ErrorCode::kConfigFileReadError);
    }

    const auto root{score::json::JsonParser{}.FromBuffer(content)};
    if (!root.has_value())
    {
        GetLogger().LogError() << "Configuration file is not valid JSON" << path;
        return score::MakeUnexpected(ErrorCode::kInvalidConfiguration);
    }
    return Parse(*root);
}

score::Result<KvsConfiguration> ConfigJsonParsingStrategy::Parse(const score::json::Any& root) const noexcept
{
    const auto root_object{root.As<score::json::Object>()};
    if (!root_object.has_value())
    {
        return Fail("Root is not an object");
    }
    const score::json::Object& document{root_object->get()};
    if (!HasOnlyKeys(document, kConfigRootKeys))
    {
        return Fail("Root has an unknown key");
    }

    const auto version_it{document.find("formatVersion")};
    if (version_it == document.end())
    {
        return Fail("formatVersion is missing");
    }
    const auto version{version_it->second.As<std::int64_t>()};
    if (!version.has_value() || (*version != kConfigFormatVersion))
    {
        return Fail("formatVersion is not supported");
    }

    const auto list_it{document.find("kvsInstances")};
    if (list_it == document.end())
    {
        return Fail("kvsInstances is missing");
    }
    const auto list{list_it->second.As<score::json::List>()};
    if (!list.has_value())
    {
        return Fail("kvsInstances is not an array");
    }

    KvsConfiguration configuration{};
    for (const auto& element : list->get())
    {
        const auto options{ParseEntry(element)};
        if (!options.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidConfiguration);
        }
        if (!configuration.Add(*options).has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidConfiguration);
        }
    }
    return configuration;
}

}  // namespace score::kvs

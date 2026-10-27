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
#include "score/kvs/internal/kvs_config_parser.hpp"
#include "score/kvs/error.hpp"
#include "score/os/stdlib.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

namespace score::mw::per::kvs
{

namespace
{

/* Entry keys accepted inside a `key_value_storage_list` element. */
constexpr std::array<std::string_view, 7> kAllowedEntryKeys{"instance_id",
                                                            "storage_path",
                                                            "need_defaults",
                                                            "need_kvs",
                                                            "snapshot_max_count",
                                                            "access_mode",
                                                            "max_storage_size_bytes"};

/* Root keys accepted at the top level of the configuration document. */
constexpr std::array<std::string_view, 2> kAllowedRootKeys{"format_version", "key_value_storage_list"};

/*
 * input: `score::json::Object` (either the document root or a `key_value_storage_list` element)
 *        and the set of key names it is allowed to contain.
 * behavior: Checks every key of `obj` against `allowed`; no mutation, no I/O.
 * output: `true` if every key is allowed, `false` on the first unknown key.
 */
bool has_only_allowed_keys(const score::json::Object& obj, const std::vector<std::string_view>& allowed)
{
    for (const auto& [key, value] : obj)
    {
        (void)value;
        const std::string_view key_view = key.GetAsStringView();
        bool found = false;
        for (const auto& allowed_key : allowed)
        {
            if (key_view == allowed_key)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            return false;
        }
    }
    return true;
}

} /* namespace */

/*
 * input: the error code and a user message describing the violation.
 * behavior: Logs the error at ERROR level; no other side effect.
 * output: the corresponding `score::Unexpected`, convertible to any `score::Result<T>`.
 */
score::Unexpected KvsConfigParser::fail(ErrorCode code, std::string_view user_message) const
{
    const auto error = MakeError(code, user_message);
    logger_->LogError() << "KVS config:" << error.Message() << error.UserMessage();
    return score::Unexpected{error};
}

/*
 * input: a `key_value_storage_list` element and the name of a `need_*` field.
 * behavior: Looks up `key` and maps "optional"/"required" to `KvsConfigNeedMode`; no mutation.
 * output: the mode, `std::nullopt` if `key` is absent, or `ErrorCode::InvalidConfiguration` for a
 *         non-string or unknown value.
 */
score::Result<std::optional<KvsConfigNeedMode>> KvsConfigParser::parse_need_mode(const score::json::Object& obj,
                                                                                 std::string_view key) const
{
    const auto it = obj.find(key);
    if (it == obj.end())
    {
        return std::optional<KvsConfigNeedMode>{std::nullopt};
    }
    const auto str_res = it->second.As<std::string>();
    if (!str_res)
    {
        return fail(ErrorCode::InvalidConfiguration, "need_* must be a string");
    }
    const std::string_view value = str_res.value().get();
    if (value == "optional")
    {
        return std::optional<KvsConfigNeedMode>{KvsConfigNeedMode::Optional};
    }
    if (value == "required")
    {
        return std::optional<KvsConfigNeedMode>{KvsConfigNeedMode::Required};
    }
    return fail(ErrorCode::InvalidConfiguration, "unknown need_* value");
}

/*
 * input: a single element of the `key_value_storage_list` array.
 * behavior: Validates that the element is an object containing only known keys, extracts the
 *           mandatory `instance_id` and every optional field, and rejects wrong types, unknown
 *           string enum values, and out-of-range numbers. Nothing is mutated outside the result.
 * output: the parsed `KvsConfigEntry` on success; `ErrorCode::InvalidConfiguration` for any
 *         structural or semantic violation of the entry.
 */
score::Result<KvsConfigEntry> KvsConfigParser::parse_kvs_config_entry(const score::json::Any& elem) const
{
    const auto obj_res = elem.As<score::json::Object>();
    if (!obj_res)
    {
        return fail(ErrorCode::InvalidConfiguration, "config entry is not an object");
    }
    const score::json::Object& entry_obj = obj_res.value().get();

    if (!has_only_allowed_keys(entry_obj, {kAllowedEntryKeys.begin(), kAllowedEntryKeys.end()}))
    {
        return fail(ErrorCode::InvalidConfiguration, "config entry has an unknown key");
    }

    KvsConfigEntry entry{};

    const auto instance_id_it = entry_obj.find("instance_id");
    if (instance_id_it == entry_obj.end())
    {
        return fail(ErrorCode::InvalidConfiguration, "config entry is missing instance_id");
    }
    const auto instance_id_res = instance_id_it->second.As<std::int64_t>();
    if (!instance_id_res || (instance_id_res.value() < 0))
    {
        return fail(ErrorCode::InvalidConfiguration, "config entry has an invalid instance_id");
    }
    entry.instance_id = static_cast<std::size_t>(instance_id_res.value());

    if (const auto it = entry_obj.find("storage_path"); it != entry_obj.end())
    {
        const auto str_res = it->second.As<std::string>();
        if (!str_res)
        {
            return fail(ErrorCode::InvalidConfiguration, "storage_path must be a string");
        }
        entry.storage_path = str_res.value().get();
    }

    const auto need_defaults_res = parse_need_mode(entry_obj, "need_defaults");
    if (!need_defaults_res)
    {
        return score::MakeUnexpected<KvsConfigEntry>(need_defaults_res.error());
    }
    entry.need_defaults = need_defaults_res.value();

    const auto need_kvs_res = parse_need_mode(entry_obj, "need_kvs");
    if (!need_kvs_res)
    {
        return score::MakeUnexpected<KvsConfigEntry>(need_kvs_res.error());
    }
    entry.need_kvs = need_kvs_res.value();

    if (const auto it = entry_obj.find("snapshot_max_count"); it != entry_obj.end())
    {
        const auto count_res = it->second.As<std::int64_t>();
        if (!count_res || (count_res.value() < 0) ||
            (count_res.value() > static_cast<std::int64_t>(kSnapshotMaxCountLimit)))
        {
            return fail(ErrorCode::InvalidConfiguration,
                        "snapshot_max_count must be an integer within the allowed range");
        }
        entry.snapshot_max_count = static_cast<std::size_t>(count_res.value());
    }

    if (const auto it = entry_obj.find("access_mode"); it != entry_obj.end())
    {
        const auto str_res = it->second.As<std::string>();
        if (!str_res)
        {
            return fail(ErrorCode::InvalidConfiguration, "access_mode must be a string");
        }
        const std::string_view value = str_res.value().get();
        if (value == "read_write")
        {
            entry.read_only = false;
        }
        else if (value == "read_only")
        {
            entry.read_only = true;
        }
        else
        {
            return fail(ErrorCode::InvalidConfiguration, "unknown access_mode value");
        }
    }

    if (const auto it = entry_obj.find("max_storage_size_bytes"); it != entry_obj.end())
    {
        const auto size_res = it->second.As<std::int64_t>();
        if (!size_res || (size_res.value() <= 0))
        {
            return fail(ErrorCode::InvalidConfiguration, "max_storage_size_bytes must be a positive integer");
        }
        entry.max_storage_size_bytes = static_cast<std::size_t>(size_res.value());
    }

    return entry;
}

/*
 * input: none (reads the process environment and, on QNX, the file content of /proc/self/exefile).
 * behavior: Resolves the directory of the running binary via the `/proc/self/exe` symlink; falls
 *           back to reading the plain-text path from `/proc/self/exefile` (QNX) if that symlink
 *           does not exist. Performs no mutation.
 * output: the binary's parent directory as a string, or `std::nullopt` if neither mechanism is
 *         available on the current platform.
 */
std::optional<std::string> KvsConfigParser::running_binary_directory() const
{
    const auto exe_link = filesystem_.standard->ReadSymlink(score::filesystem::Path{"/proc/self/exe"});
    if (exe_link)
    {
        return exe_link.value().ParentPath().Native();
    }

    const auto exefile = filesystem_.streams->Open(score::filesystem::Path{"/proc/self/exefile"}, std::ios::in);
    if (exefile)
    {
        std::string exe_path;
        std::getline(*exefile.value(), exe_path);
        if (!exe_path.empty())
        {
            return score::filesystem::Path{exe_path}.ParentPath().Native();
        }
    }

    return std::nullopt;
}

/*
 * input: none (reads the `STORAGE_CONFIG_PATH` environment variable and the filesystem).
 * behavior: An explicitly configured `STORAGE_CONFIG_PATH` (directory or file; empty counts as
 *           unset) is resolved and must exist, otherwise this is a hard error. Without it, falls
 *           back to `<binary dir>/../etc/key_value_storage_configs.json`; a missing fallback file,
 *           or an undeterminable binary location, is not an error. No file is read or written.
 * output: - path of the configuration file, if one was found;
 *         - `std::nullopt` if no configuration file applies (no `STORAGE_CONFIG_PATH`, and the
 *           fallback file is missing or the binary location is unknown);
 *         - `ErrorCode::FileNotFound` if `STORAGE_CONFIG_PATH` names a missing target.
 */
score::Result<std::optional<std::string>> KvsConfigParser::discover_kvs_config_file_path() const
{
    const auto stdlib = score::os::Stdlib::Default();
    const char* const env_value = stdlib->getenv(std::string(kStorageConfigPathEnvVar).c_str());
    if ((env_value != nullptr) && (*env_value != '\0'))
    {
        score::filesystem::Path candidate{std::string{env_value}};
        const auto is_directory = filesystem_.standard->IsDirectory(candidate);
        if (is_directory && is_directory.value())
        {
            candidate /= std::string(kKvsConfigFileName);
        }
        const auto exists = filesystem_.standard->Exists(candidate);
        if (!exists || !exists.value())
        {
            logger_->LogError() << "KVS config: configured path does not exist:" << candidate.Native();
            return fail(ErrorCode::FileNotFound, "configured path does not exist");
        }
        return std::make_optional(candidate.Native());
    }

    const std::optional<std::string> bin_dir = running_binary_directory();
    if (!bin_dir.has_value())
    {
        return std::optional<std::string>{std::nullopt};
    }

    const score::filesystem::Path candidate = score::filesystem::Path{bin_dir.value()} / ".." / "etc" /
                                              score::filesystem::Path{std::string(kKvsConfigFileName)};
    const auto exists = filesystem_.standard->Exists(candidate);
    if (!exists || !exists.value())
    {
        return std::optional<std::string>{std::nullopt};
    }
    return std::make_optional(candidate.Native());
}

/*
 * input: filesystem path of a presumed configuration-by-file document.
 * behavior: Reads the whole file, parses it as JSON, and strictly validates its structure: exactly
 *           `format_version` (must be `1`) and `key_value_storage_list` (an array) at the root,
 *           every list element a validated entry (see `parse_kvs_config_entry`), and no duplicate
 *           `instance_id` across entries. Validates the whole file, not only entries matching a
 *           particular instance. The file is only read, never written.
 * output: all validated entries on success; `ErrorCode::KvsFileReadError` if the file could not be
 *         opened/read, `ErrorCode::JsonParserError` if it is not valid JSON, or
 *         `ErrorCode::InvalidConfiguration` for any structural or semantic violation.
 */
score::Result<std::vector<KvsConfigEntry>> KvsConfigParser::parse_kvs_config_file(const std::string& path) const
{
    const auto in = filesystem_.streams->Open(score::filesystem::Path{path}, std::ios::in);
    if (!in)
    {
        logger_->LogError() << "KVS config: failed to open config file:" << path;
        return fail(ErrorCode::KvsFileReadError, "failed to open config file");
    }
    const std::string data{std::istreambuf_iterator<char>(*in.value()), std::istreambuf_iterator<char>()};

    score::json::JsonParser parser;
    auto any_res = parser.FromBuffer(data);
    if (!any_res)
    {
        logger_->LogError() << "KVS config: failed to parse config file as JSON:" << path;
        return fail(ErrorCode::JsonParserError, "failed to parse config file as JSON");
    }
    const score::json::Any root = std::move(any_res).value();

    const auto root_obj_res = root.As<score::json::Object>();
    if (!root_obj_res)
    {
        return fail(ErrorCode::InvalidConfiguration, "config root is not an object");
    }
    const score::json::Object& root_obj = root_obj_res.value().get();

    if (!has_only_allowed_keys(root_obj, {kAllowedRootKeys.begin(), kAllowedRootKeys.end()}))
    {
        return fail(ErrorCode::InvalidConfiguration, "config root has an unknown key");
    }

    const auto format_version_it = root_obj.find("format_version");
    if (format_version_it == root_obj.end())
    {
        return fail(ErrorCode::InvalidConfiguration, "config is missing format_version");
    }
    const auto format_version_res = format_version_it->second.As<std::int64_t>();
    if (!format_version_res || (format_version_res.value() != 1))
    {
        return fail(ErrorCode::InvalidConfiguration, "unsupported format_version");
    }

    const auto list_it = root_obj.find("key_value_storage_list");
    if (list_it == root_obj.end())
    {
        return fail(ErrorCode::InvalidConfiguration, "config is missing key_value_storage_list");
    }
    const auto list_res = list_it->second.As<score::json::List>();
    if (!list_res)
    {
        return fail(ErrorCode::InvalidConfiguration, "key_value_storage_list must be an array");
    }

    std::vector<KvsConfigEntry> entries;
    entries.reserve(list_res.value().get().size());
    for (const auto& elem : list_res.value().get())
    {
        auto entry_res = parse_kvs_config_entry(elem);
        if (!entry_res)
        {
            return score::MakeUnexpected<std::vector<KvsConfigEntry>>(entry_res.error());
        }
        for (const auto& existing : entries)
        {
            if (existing.instance_id == entry_res.value().instance_id)
            {
                return fail(ErrorCode::InvalidConfiguration, "duplicate instance_id");
            }
        }
        entries.emplace_back(std::move(entry_res.value()));
    }

    return entries;
}

/*
 * input: the `InstanceId::id` of the KVS instance about to be opened.
 * behavior: Discovers whether a configuration-by-file document applies at all, and if so parses
 *           and strictly validates it in full before looking for an entry matching `instance_id`.
 *           Performs no mutation; intended to be called once per `KvsBuilder::build()`.
 * output: - the entry whose `instance_id` matches, if one exists;
 *         - `std::nullopt` if no configuration file applies, or the file has no entry for
 *           `instance_id` (the caller then uses pure code configuration);
 *         - the error from `discover_kvs_config_file_path()` (`ErrorCode::FileNotFound`) or from
 *           `parse_kvs_config_file()` (`ErrorCode::KvsFileReadError`, `ErrorCode::JsonParserError`,
 *           `ErrorCode::InvalidConfiguration`). The whole file is rejected on any such error, even
 *           if the broken part belongs to a different instance.
 */
score::Result<std::optional<KvsConfigEntry>> KvsConfigParser::load_entry(std::size_t instance_id)
{
    auto path_res = discover_kvs_config_file_path();
    if (!path_res)
    {
        return score::MakeUnexpected<std::optional<KvsConfigEntry>>(path_res.error());
    }
    if (!path_res.value().has_value())
    {
        return std::optional<KvsConfigEntry>{std::nullopt};
    }

    auto entries_res = parse_kvs_config_file(path_res.value().value());
    if (!entries_res)
    {
        return score::MakeUnexpected<std::optional<KvsConfigEntry>>(entries_res.error());
    }

    for (auto& entry : entries_res.value())
    {
        if (entry.instance_id == instance_id)
        {
            return std::make_optional(std::move(entry));
        }
    }

    return std::optional<KvsConfigEntry>{std::nullopt};
}

} /* namespace score::mw::per::kvs */

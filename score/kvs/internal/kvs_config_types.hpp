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
#ifndef SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_TYPES_HPP
#define SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

/*
 * Shared value types for deployment-time "configuration by file" discovery and parsing for the
 * Key-Value Store (KVS). Kept separate from `IKvsConfigParser` so that both the real parser
 * (`KvsConfigParser`) and its test double (`KvsConfigParserMock`) can include just the types
 * without pulling in the interface declaration.
 */
namespace score::mw::per::kvs
{

/* Name of the environment variable naming the configuration directory or file. */
inline constexpr std::string_view kStorageConfigPathEnvVar = "STORAGE_CONFIG_PATH";

/* Fixed filename of the configuration-by-file document, used standalone or appended to a
 * directory named by the environment variable or to the binary-relative fallback directory. */
inline constexpr std::string_view kKvsConfigFileName = "key_value_storage_configs.json";

/* Largest accepted `snapshot_max_count`; 0 disables snapshot rotation.
 * Mirrors `KVS_MAX_SNAPSHOTS_UPPER_BOUND` from kvs.hpp. It cannot be defined in terms of that
 * constant here: `kvs_core` (kvs.hpp/kvs.cpp) depends on `kvs_config_parser` (this header), so
 * including kvs.hpp back from here would create a Bazel dependency cycle. Keep the value in sync
 * manually; the static_assert below only guards against accidental drift of this local copy. */
inline constexpr std::size_t kSnapshotMaxCountLimit{3U};

/* Need-mode for a KVS resource (defaults file / KVS file) as configurable from the configuration
 * file. Mirrors the semantics of `OpenNeedDefaults`/`OpenNeedKvs` from kvs.hpp without depending
 * on that header. */
enum class KvsConfigNeedMode : std::uint8_t
{
    Optional, /* Resource may be absent; an empty storage is used instead. */
    Required, /* Resource must be present. */
};

/*
 * A single, already-validated entry of the `key_value_storage_list` array.
 * Every field other than `instance_id` is optional: an unset field means the configuration file
 * does not override that setting, and `KvsBuilder::build()` keeps its own builder-set or
 * built-in-default value for it.
 */
struct KvsConfigEntry
{
    std::size_t instance_id;                            ///< Matched against `KvsBuilder(InstanceId)`.
    std::optional<std::string> storage_path;            ///< Overrides `KvsBuilder::dir()`.
    std::optional<KvsConfigNeedMode> need_defaults;     ///< Overrides `KvsBuilder::need_defaults_flag()`.
    std::optional<KvsConfigNeedMode> need_kvs;          ///< Overrides `KvsBuilder::need_kvs_flag()`.
    std::optional<std::size_t> snapshot_max_count;      ///< Overrides the runtime snapshot limit (0..3).
    std::optional<bool> read_only;                      ///< Overrides the read-only flag (`access_mode`).
    std::optional<std::size_t> max_storage_size_bytes;  ///< Overrides the max serialized payload size.
};

} /* namespace score::mw::per::kvs */

#endif /* SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_TYPES_HPP */

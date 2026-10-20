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
/// @file
/// @brief Public KVS types and constants defining limits and defaults for the types and API.

#ifndef SCORE_KVS_KVS_TYPES_H
#define SCORE_KVS_KVS_TYPES_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace score::kvs
{
/// @brief Maximum allowed key length in bytes.
/// @note comp_req__kvs__key_length
constexpr std::size_t kMaxKeyLengthBytes{32U};

/// @deprecated Use `kMaxKeyLengthBytes` instead.
[[deprecated("Use kMaxKeyLengthBytes instead. This name is removed in a future release.")]]
constexpr std::size_t KVS_MAX_KEY_LENGTH_BYTES{kMaxKeyLengthBytes};

/// @brief Upper bound of the configurable maximum number of snapshots.
///
/// The configurable maximum number of snapshots is limited to the range 0..kMaxSnapshotsUpperBound.
/// A value of 0 disables snapshot operations.
/// @note comp_req__kvs__snapshot_max_num_cfg
constexpr std::size_t kMaxSnapshotsUpperBound{3U};

/// @deprecated Use `kMaxSnapshotsUpperBound` instead.
[[deprecated("Use kMaxSnapshotsUpperBound instead. This name is removed in a future release.")]]
constexpr std::size_t KVS_MAX_SNAPSHOTS_UPPER_BOUND{kMaxSnapshotsUpperBound};

/// @brief Default maximum number of snapshots.
///
/// It can be overridden per instance at init-time via KvsBuilder::SnapshotMaxCount().
/// @note comp_req__kvs__snapshot_max_num_cfg
constexpr std::size_t kDefaultMaxSnapshots{3U};

/// @deprecated Use `kDefaultMaxSnapshots` instead.
[[deprecated("Use kDefaultMaxSnapshots instead. This name is removed in a future release.")]]
constexpr std::size_t KVS_DEFAULT_MAX_SNAPSHOTS{kDefaultMaxSnapshots};

/// @brief Maximum number of KVS instances in one process.
///
/// Valid instance IDs are in the range 0..kMaxInstanceCount - 1.
/// `KvsBuilder::Build()` fails with `kInvalidInstanceId` for any other ID.
constexpr std::size_t kMaxInstanceCount{10UL};

/// @brief Identifier of a KVS instance.
///
/// Must be in the range 0..kMaxInstanceCount - 1.
// codeql::misra_deviation_next_line(kvs-special-members)
struct InstanceId
{
    /// @brief Constructs an instance identifier.
    ///
    /// Not explicit to allow implicit construction, e.g. `function(0)` instead of `function(InstanceId(0))`.
    /// @param id Numeric value of the identifier.
    // codeql::misra_deviation_next_line(kvs-id-implicit-ctor)
    InstanceId(std::size_t id) noexcept : id{id} {}

    /// Numeric value of the identifier.
    std::size_t id;
};

/// @brief Identifier of a KVS snapshot.
// codeql::misra_deviation_next_line(kvs-special-members)
struct SnapshotId
{
    /// @brief Constructs a snapshot identifier.
    ///
    /// Not explicit to allow implicit construction, e.g. `function(0)` instead of `function(SnapshotId(0))`.
    /// @param id Numeric value of the identifier.
    // codeql::misra_deviation_next_line(kvs-id-implicit-ctor)
    SnapshotId(std::size_t id) noexcept : id{id} {}

    /// Numeric value of the identifier.
    std::size_t id;
};

/// @brief Whether a JSON file must exist when it is opened.
enum class OpenJsonNeedFile : std::uint8_t
{
    /// The file must already exist.
    kRequired = 0,
    /// If the file doesn't exist, start with empty data.
    kOptional = 1,

    // Deprecated enumerators.
    // Each one is an alias of its k-prefixed replacement and is removed in a future release.
    Required [[deprecated("Use kRequired instead. This name is removed in a future release.")]] = kRequired,
    Optional [[deprecated("Use kOptional instead. This name is removed in a future release.")]] = kOptional,
};

/// @brief Whether default values must be available when opening a KVS.
enum class KvsDefaults : std::uint8_t
{
    /// Defaults must be available.
    kRequired = 0,
    /// Use empty defaults if none are available.
    kOptional = 1,
    /// Defaults are not loaded.
    kIgnored = 2,

    // Deprecated enumerators.
    // Each one is an alias of its k-prefixed replacement and is removed in a future release.
    Required [[deprecated("Use kRequired instead. This name is removed in a future release.")]] = kRequired,
    Optional [[deprecated("Use kOptional instead. This name is removed in a future release.")]] = kOptional,
    Ignored [[deprecated("Use kIgnored instead. This name is removed in a future release.")]] = kIgnored,
};

/// @deprecated Use `KvsDefaults` instead.
using OpenNeedDefaults [[deprecated("Use KvsDefaults instead. This name is removed in a future release.")]] =
    KvsDefaults;

/// @brief Whether KVS data must already exist when opening a KVS.
enum class KvsLoad : std::uint8_t
{
    /// KVS data must already exist.
    kRequired = 0,
    /// Use an empty KVS if no KVS data is available.
    kOptional = 1,
    /// KVS data is not loaded.
    kIgnored = 2,

    // Deprecated enumerators.
    // Each one is an alias of its k-prefixed replacement and is removed in a future release.
    Required [[deprecated("Use kRequired instead. This name is removed in a future release.")]] = kRequired,
    Optional [[deprecated("Use kOptional instead. This name is removed in a future release.")]] = kOptional,
    Ignored [[deprecated("Use kIgnored instead. This name is removed in a future release.")]] = kIgnored,
};

/// @deprecated Use `KvsLoad` instead.
using OpenNeedKvs [[deprecated("Use KvsLoad instead. This name is removed in a future release.")]] = KvsLoad;

/// @brief Open parameters.
struct KvsOpenParameters final
{
    /// KVS instance identifier, in the range 0..kMaxInstanceCount - 1.
    std::size_t instance_id;
    /// Identifier of the snapshot to load on open (0 is the current KVS data).
    std::size_t snapshot_id;
    /// Whether KVS data is required, optional or ignored when opening.
    KvsLoad current_load_policy;
    /// Whether default values are required, optional or ignored when opening.
    KvsDefaults default_load_policy;
    /// Directory where KVS files are stored ("" or "." for the current directory).
    std::string working_dir;
    /// Maximum total size of this instance's files in bytes; std::nullopt means unlimited.
    std::optional<std::size_t> storage_quota;
    /// Maximum number of snapshots kept, in the range 0..kMaxSnapshotsUpperBound; 0 disables snapshots.
    std::size_t snapshot_count_limit;
};

/// @brief Compares the kvs open options.
bool operator==(const KvsOpenParameters& v1, const KvsOpenParameters& v2) noexcept;
bool operator!=(const KvsOpenParameters& v1, const KvsOpenParameters& v2) noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_KVS_TYPES_H

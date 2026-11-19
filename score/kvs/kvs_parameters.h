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
#ifndef SCORE_KVS_KVS_PARAMETERS_H
#define SCORE_KVS_KVS_PARAMETERS_H

#include "score/kvs/kvs.h"
#include "score/mw/log/logging.h"
#include <optional>
#include <string>

namespace score::kvs
{

/// @brief Options of one KVS instance.
///
/// The instance ID has no default and is set by the constructor.
/// The default member values are the built-in defaults of every other option.
struct KvsParameters final
{
    /// @brief Constructs the options of an instance with the built-in defaults.
    /// @param instance_id Identifier of the instance, in the range 0..kMaxInstanceCount - 1.
    explicit KvsParameters(const InstanceId& instance_id) noexcept;

    /// KVS instance identifier, in the range 0..kMaxInstanceCount - 1.
    std::size_t instance_id;
    /// Identifier of the snapshot to load on open (0 is the current KVS data).
    std::size_t snapshot_id{0U};
    /// Whether KVS data is required, optional or ignored when opening.
    KvsLoad current_load_policy{KvsLoad::kOptional};
    /// Whether default values are required, optional or ignored when opening.
    KvsDefaults default_load_policy{KvsDefaults::kOptional};
    /// Directory where KVS files are stored ("" or "." for the current directory).
    std::string working_dir{kDefaultWorkingDir};
    /// Maximum total size of this instance's files in bytes; std::nullopt means unlimited.
    std::optional<size_t> storage_quota{std::nullopt};
    /// Maximum number of snapshots kept, in the range 0..kMaxSnapshotsUpperBound; 0 disables snapshots.
    std::size_t snapshot_count_limit{kDefaultMaxSnapshots};
};

score::mw::log::LogStream& operator<<(score::mw::log::LogStream& stream, const KvsParameters& params);

/// @brief Configures the options of a KVS instance.
///
/// The builder only collects options.
/// Add the built options to a `KvsConfiguration`, call `Initialize()` once and then `Get()` a handle:
/// @code
/// KvsConfiguration configuration;
/// configuration.Add(KvsParametersBuilder{InstanceId{0U}}.WorkingDir("/var/lib/app").Build());
/// Initialize(configuration);
/// auto kvs{Get(InstanceId{0U})};
/// @endcode
class KvsParametersBuilder final
{
  public:
    /// @brief Constructs a KvsParametersBuilder for the given KVS instance.
    /// @param instance_id Unique identifier for the KVS instance, in the range 0..kMaxInstanceCount - 1.
    explicit KvsParametersBuilder(const InstanceId& instance_id) noexcept;

    /// @brief Specifies the snapshot identifier to be loaded.
    /// @param snapshot_id Unique identifier for the KVS snapshot.
    /// @return Reference to this builder (for chaining).
    KvsParametersBuilder& Snapshot(const SnapshotId& snapshot_id) noexcept;

    /// @brief Specifies whether default values must be loaded.
    /// @param mode Whether default values are required, optional or ignored.
    /// @return Reference to this builder (for chaining).
    KvsParametersBuilder& Defaults(const KvsDefaults& mode) noexcept;

    /// @brief Specifies whether KVS data must exist when opening the KVS.
    /// @param mode Whether KVS data is required, optional or ignored.
    /// @return Reference to this builder (for chaining).
    KvsParametersBuilder& KvsLoad(const score::kvs::KvsLoad& mode) noexcept;

    /// @brief Specifies the directory where KVS files are stored.
    /// @param working_dir The directory path.
    /// Use "" or "." for the current directory.
    /// @return Reference to this builder (for chaining).
    KvsParametersBuilder& WorkingDir(const std::string& working_dir) noexcept;

    /// @brief Specifies the maximum storage size for the KVS in bytes.
    /// @param max_storage_bytes The maximum allowed total storage size in bytes.
    /// The limit applies to the files of this KVS instance only, not to other instances.
    /// When this option is not used, no storage limit is enforced.
    /// @return Reference to this builder (for chaining).
    KvsParametersBuilder& MaxStorageBytes(const std::optional<size_t>& max_storage_bytes) noexcept;

    /// @brief Specifies the maximum number of snapshots the KVS maintains.
    ///
    /// The maximum number of snapshots is configurable per instance in the range 0..kMaxSnapshotsUpperBound.
    /// When this option is not used, kDefaultMaxSnapshots is applied.
    /// @note comp_req__kvs__snapshot_max_num_cfg
    ///
    /// @param max_count Maximum number of snapshots.
    /// Values above kMaxSnapshotsUpperBound are clamped to it by this call, which logs a warning.
    /// A value of 0 disables snapshot operations.
    /// Flush() still persists the current KVS data.
    /// @return Reference to this builder (for chaining).
    KvsParametersBuilder& SnapshotMaxCount(const std::size_t max_count) noexcept;

    /// @brief Returns the configured options.
    ///
    /// Reads no files and does not validate the options.
    /// @return A copy of the options.
    KvsParameters Build() const noexcept;

  private:
    /// Options being configured.
    KvsParameters params_;
};

}  // namespace score::kvs

#endif  // SCORE_KVS_KVS_PARAMETERS_H

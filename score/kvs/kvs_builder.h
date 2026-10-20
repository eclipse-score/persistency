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
#ifndef SCORE_KVS_KVS_BUILDER_H
#define SCORE_KVS_KVS_BUILDER_H

#include "score/kvs/kvs.h"
#include "score/mw/log/logging.h"
#include <memory>
#include <optional>
#include <string>

namespace score::kvs
{

score::mw::log::LogStream& operator<<(score::mw::log::LogStream& stream, const KvsOpenParameters& params);

/// @brief Configures the open options of a KVS instance and builds a shared pointer to it.
class KvsBuilder final
{
  public:
    /// @brief Constructs a KvsBuilder for the given KVS instance.
    /// @param instance_id Unique identifier for the KVS instance, in the range 0..kMaxInstanceCount - 1.
    explicit KvsBuilder(const InstanceId& instance_id) noexcept;

    /// @brief Specifies the snapshot identifier to be loaded.
    /// @param snapshot_id Unique identifier for the KVS snapshot.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& Snapshot(const SnapshotId& snapshot_id) noexcept;

    /// @brief Specifies whether default values must be loaded.
    /// @param mode Whether default values are required, optional or ignored.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& Defaults(const KvsDefaults& mode) noexcept;

    /// @brief Specifies whether KVS data must exist when opening the KVS.
    /// @param mode Whether KVS data is required, optional or ignored.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& KvsLoad(const score::kvs::KvsLoad& mode) noexcept;

    /// @brief Specifies the directory where KVS files are stored.
    /// @param working_dir The directory path.
    /// Use "" or "." for the current directory.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& WorkingDir(const std::string& working_dir) noexcept;

    /// @brief Specifies the maximum storage size for the KVS in bytes.
    /// @param max_storage_bytes The maximum allowed total storage size in bytes.
    /// The limit applies to the files of this KVS instance only, not to other instances.
    /// When this option is not used, no storage limit is enforced.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& MaxStorageBytes(const std::optional<size_t>& max_storage_bytes) noexcept;

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
    KvsBuilder& SnapshotMaxCount(const std::size_t max_count) noexcept;

    /// @brief Builds and opens the Kvs instance with the configured options.
    ///
    /// Builds for the same instance ID and options return the same object retained by the registry.
    /// After `Unregister()`, a later build creates a new object; see `Unregister()` for details.
    ///
    /// @return The opened store on success, or an ErrorCode on failure:
    /// - `kInvalidInstanceId` if the instance ID is not below `kMaxInstanceCount`.
    /// - `kInstanceParametersMismatch` if the instance is already open with different parameters.
    /// - `kKvsFileReadError` if a required defaults or KVS file cannot be read.
    /// - `kKvsHashFileReadError` if the hash file of an existing defaults or KVS file cannot be read.
    /// - `kValidationFailed` if a defaults or KVS file does not match its hash.
    /// - `kJsonParserError` if a defaults or KVS file is not a valid JSON object.
    /// - `kInvalidKey` if a defaults or KVS file contains an invalid key.
    /// - `kInvalidValueType` if a defaults or KVS file contains a value of an unsupported type.
    score::Result<std::shared_ptr<Kvs>> Build() const noexcept;

    // Deprecated snake_case API.
    // Each entry forwards to its PascalCase replacement and is removed in a future release.

    /// @deprecated Use `Snapshot()` instead.
    [[deprecated("Use Snapshot() instead. This name is removed in a future release.")]]
    KvsBuilder& snapshot(const SnapshotId& snapshot_id) noexcept
    {
        return Snapshot(snapshot_id);
    }

    /// @deprecated Use `Defaults()` instead.
    [[deprecated("Use Defaults() instead. This name is removed in a future release.")]]
    KvsBuilder& need_defaults_flag(const KvsDefaults& flag) noexcept
    {
        return Defaults(flag);
    }

    /// @deprecated Use `KvsLoad()` instead.
    [[deprecated("Use KvsLoad() instead. This name is removed in a future release.")]]
    KvsBuilder& need_kvs_flag(const score::kvs::KvsLoad& flag) noexcept
    {
        return KvsLoad(flag);
    }

    /// @deprecated Use `WorkingDir()` instead.
    [[deprecated("Use WorkingDir() instead. This name is removed in a future release.")]]
    KvsBuilder& dir(const std::string& dir_path) noexcept
    {
        return WorkingDir(dir_path);
    }

    /// @deprecated Use `MaxStorageBytes()` instead.
    [[deprecated("Use MaxStorageBytes() instead. This name is removed in a future release.")]]
    KvsBuilder& max_storage_bytes(const std::optional<size_t>& max_storage_bytes) noexcept
    {
        return MaxStorageBytes(max_storage_bytes);
    }

    /// @deprecated Use `SnapshotMaxCount()` instead.
    [[deprecated("Use SnapshotMaxCount() instead. This name is removed in a future release.")]]
    KvsBuilder& snapshot_max_count(const std::size_t max_count) noexcept
    {
        return SnapshotMaxCount(max_count);
    }

    /// @deprecated Use `Build()` instead.
    [[deprecated("Use Build() instead. This name is removed in a future release.")]]
    score::Result<std::shared_ptr<Kvs>> build() const noexcept
    {
        return Build();
    }

  private:
    /// Open options.
    KvsOpenParameters params_;
};

/// @brief Removes the instance registered under the given ID from the registry.
///
/// Existing pointers remain valid and retain their in-memory data. A later build creates a new instance and
/// reloads its data according to the requested options. No pending changes are flushed or discarded.
/// Old and new instances are not synchronized with each other and may access the same files independently;
/// callers must coordinate their use to avoid conflicting writes.
///
/// @warning Experimental: the signature, semantics and location of this function may change or be removed
/// in a future release without a deprecation period.
/// Open points:
/// - Any caller can unregister an instance shared with other users of the same ID.
/// - Whether old and new instances should be coordinated instead of leaving it to callers.
/// - Whether pending changes should be flushed or discarded implicitly.
/// - Final placement (free function, builder or a future public KVS interface).
/// - Rust counterpart.
///
/// @param instance_id Identifier of the instance to remove.
/// @return Nothing on success, or `kInvalidInstanceId` if the ID is out of range or no instance is registered under it.
score::Result<void> Unregister(const InstanceId& instance_id) noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_KVS_BUILDER_H

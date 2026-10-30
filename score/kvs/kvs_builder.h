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
#include <optional>
#include <string>

namespace score::kvs
{

/// @brief Builder for opening a KVS object.
///
/// This class configures the options for opening a KVS instance.
/// For example, whether default values are required or whether the KVS data must already exist.
///
/// @note You don't need to include any other header files to use the KVS.
/// For documentation of the KVS functions, refer to the kvs.h documentation.
///
/// Example usage:
/// @code
/// #include <iostream>
/// #include "score/kvs/kvs_builder.h"
///
/// using namespace score::kvs;
///
/// int main() {
///   // Open kvs
///   auto open_result = KvsBuilder(0)
///                       .Defaults(KvsDefaults::kRequired)
///                       .KvsLoad(KvsLoad::kRequired)
///                       .Build();
///   if (!open_result) return 1;
///   Kvs kvs = std::move(open_result.value());
///
///   // Set and get a value
///   kvs.SetValue("pi", KvsValue(3.14));
///   auto get_result = kvs.GetValue("pi");
///
///   // Delete a key
///   kvs.RemoveKey("pi");
///   std::cout << "has pi? " << (kvs.KeyExists("pi").value_or(false) ? "yes" : "no") << "\n";
///
///   return 0;
/// }
/// @endcode
class KvsBuilder final
{
  public:
    /// @brief Constructs a KvsBuilder for the given KVS instance.
    /// @param instance_id Unique identifier for the KVS instance.
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
    /// Values above kMaxSnapshotsUpperBound are clamped to it when the KVS is built.
    /// A value of 0 disables snapshot operations.
    /// Flush() still persists the current KVS data.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& SnapshotMaxCount(const std::size_t max_count) noexcept;

    /// @brief Builds and opens the Kvs instance with the configured options.
    ///
    /// Internally calls Kvs::Open() with the selected flags and directory.
    ///
    /// @return The opened store on success, or an ErrorCode on failure.
    score::Result<Kvs> Build() const noexcept;

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
    score::Result<Kvs> build() const noexcept
    {
        return Build();
    }

  private:
    /// ID of the KVS instance.
    InstanceId instance_id_;

    /// ID of the KVS snapshot.
    SnapshotId snapshot_id_;

    /// Whether default values are required.
    KvsDefaults defaults_;

    /// Whether an existing KVS is required.
    score::kvs::KvsLoad kvs_load_;

    /// Directory where the KVS files are stored.
    std::string working_dir_;

    /// Maximum total storage size in bytes.
    /// Unset means no limit.
    std::optional<size_t> maximum_storage_bytes_;

    /// Maximum number of snapshots to maintain.
    std::size_t max_snapshots_;
};

}  // namespace score::kvs

#endif  // SCORE_KVS_KVS_BUILDER_H

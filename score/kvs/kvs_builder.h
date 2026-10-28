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
///   auto open_res = KvsBuilder(0)
///                       .need_defaults_flag(OpenNeedDefaults::Required)
///                       .need_kvs_flag(OpenNeedKvs::Required)
///                       .build();
///   if (!open_res) return 1;
///   Kvs kvs = std::move(open_res.value());
///
///   // Set and get a value
///   kvs.set_value("pi", KvsValue(3.14));
///   auto get_res = kvs.get_value("pi");
///
///   // Delete a key
///   kvs.remove_key("pi");
///   std::cout << "has pi? " << (kvs.key_exists("pi").value_or(false) ? "yes" : "no") << "\n";
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
    KvsBuilder& snapshot(const SnapshotId& snapshot_id) noexcept;

    /// @brief Specifies whether default values must be loaded.
    /// @param flag Whether default values are required, optional or ignored.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& need_defaults_flag(const OpenNeedDefaults& flag) noexcept;

    /// @brief Specifies whether KVS data must exist when opening the KVS.
    /// @param flag Whether KVS data is required, optional or ignored.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& need_kvs_flag(const OpenNeedKvs& flag) noexcept;

    /// @brief Specifies the directory where KVS files are stored.
    /// @param dir_path The directory path.
    /// Use "" or "." for the current directory.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& dir(const std::string& dir_path) noexcept;

    /// @brief Specifies the maximum storage size for the KVS in bytes.
    /// @param max_storage_bytes The maximum allowed total storage size in bytes.
    /// When this option is not used, no storage limit is enforced.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& max_storage_bytes(const std::optional<size_t>& max_storage_bytes) noexcept;

    /// @brief Specifies the maximum number of snapshots the KVS maintains.
    ///
    /// The maximum number of snapshots is configurable per instance in the range 0..KVS_MAX_SNAPSHOTS_UPPER_BOUND.
    /// When this option is not used, KVS_DEFAULT_MAX_SNAPSHOTS is applied.
    /// @note comp_req__kvs__snapshot_max_num_cfg
    ///
    /// @param max_count Maximum number of snapshots.
    /// Values above KVS_MAX_SNAPSHOTS_UPPER_BOUND are clamped to it when the KVS is built.
    /// A value of 0 disables snapshot operations.
    /// flush() still persists the current KVS data.
    /// @return Reference to this builder (for chaining).
    KvsBuilder& snapshot_max_count(const std::size_t max_count) noexcept;

    /// @brief Builds and opens the Kvs instance with the configured options.
    ///
    /// Internally calls Kvs::open() with the selected flags and directory.
    ///
    /// @return The opened store on success, or an ErrorCode on failure.
    score::Result<Kvs> build() const noexcept;

  private:
    /// ID of the KVS instance.
    InstanceId instance_id_;

    /// ID of the KVS snapshot.
    SnapshotId snapshot_id_;

    /// Whether default values are required.
    OpenNeedDefaults need_defaults_;

    /// Whether an existing KVS is required.
    OpenNeedKvs need_kvs_;

    /// Directory where the KVS files are stored.
    std::string directory_;

    /// Maximum total storage size in bytes.
    /// Unset means no limit.
    std::optional<size_t> maximum_storage_bytes_;

    /// Maximum number of snapshots to maintain.
    std::size_t max_snapshots_;
};

}  // namespace score::kvs

#endif  // SCORE_KVS_KVS_BUILDER_H

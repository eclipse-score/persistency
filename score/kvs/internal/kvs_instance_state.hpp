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
#ifndef SCORE_LIB_KVS_INTERNAL_KVS_STATE_HPP
#define SCORE_LIB_KVS_INTERNAL_KVS_STATE_HPP

#include "score/filesystem/filesystem.h"
#include "score/kvs/kvs_value.h"

#include <memory>
#include <mutex>
#include <optional>

namespace score::kvs
{
struct KvsInstanceState;

/// @brief Maximum number or instances.
constexpr std::size_t KVS_MAX_INSTANCE_COUNT{10UL};

/// @brief Registry of shared KVS instance states, indexed by instance ID.
struct KvsInstanceStateRegistry final
{
  public:
    using Entry = std::shared_ptr<KvsInstanceState>;
    using Array = std::array<Entry, KVS_MAX_INSTANCE_COUNT>;

    KvsInstanceStateRegistry(const KvsInstanceStateRegistry&) = delete;
    KvsInstanceStateRegistry(KvsInstanceStateRegistry&&) = delete;

    KvsInstanceStateRegistry& operator=(const KvsInstanceStateRegistry&) = delete;
    KvsInstanceStateRegistry& operator=(KvsInstanceStateRegistry&&) = delete;

    /// @brief Returns the process-wide registry instance.
    static KvsInstanceStateRegistry& GetInstance();

    /// @brief Reset registy. For test use only.
    void Reset();

    /// Protects access to members.
    std::mutex mutex;

    /// Array.
    Array array;

  private:
    KvsInstanceStateRegistry() = default;
};

/**
 * @brief Shared in-memory state for a KVS backing store.
 *
 * Handles opened for the same filename prefix share this state, including the
 * snapshot and storage limits. The storage limit applies to this store's files,
 * not to storage used by other KVS instances.
 */
struct KvsInstanceState final
{
    /// Protects access to the shared state.
    std::mutex mutex;
    /// Instance ID that has been specified during creation.
    std::size_t instance_id{};
    /// Snapshot ID that has been specified during creation.
    std::size_t snapshot_id{};
    /// Values in the current snapshot.
    KvsMap current_values{};
    /// Default values loaded for this store.
    KvsMap default_values{};
    /// Prefix used to construct this store's file paths.
    score::filesystem::Path filename_prefix{};
    /// Maximum number of snapshots maintained by this instance.
    /// @note comp_req__kvs__snapshot_max_num_cfg
    std::size_t max_snapshots{};
    /// Optional total storage limit for this store's files.
    std::optional<size_t> max_storage_bytes{std::nullopt};
};

} /* namespace score::kvs */

#endif  // SCORE_LIB_KVS_INTERNAL_KVS_HELPER_HPP

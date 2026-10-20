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
#ifndef SCORE_KVS_INTERNAL_KVS_INSTANCE_STATE_H
#define SCORE_KVS_INTERNAL_KVS_INSTANCE_STATE_H

#include "score/filesystem/filesystem.h"
#include "score/kvs/kvs_value.h"

#include <memory>
#include <mutex>
#include <optional>

namespace score::kvs
{
struct KvsInstanceState;

/// @brief Maximum number of instances.
constexpr std::size_t kMaxInstanceCount{10UL};

/// @brief Registry of shared KVS instance states, indexed by instance ID.
struct KvsInstanceStateRegistry final
{
  public:
    using Entry = std::shared_ptr<KvsInstanceState>;
    using Array = std::array<Entry, kMaxInstanceCount>;

    KvsInstanceStateRegistry(const KvsInstanceStateRegistry&) = delete;
    KvsInstanceStateRegistry(KvsInstanceStateRegistry&&) = delete;

    KvsInstanceStateRegistry& operator=(const KvsInstanceStateRegistry&) = delete;
    KvsInstanceStateRegistry& operator=(KvsInstanceStateRegistry&&) = delete;

    /// @brief Returns the process-wide registry instance.
    static KvsInstanceStateRegistry& GetInstance();

    /// @brief Reset registry. For test use only.
    void Reset();

    /// Protects access to members.
    std::mutex mutex;

    /// Array.
    Array array;

  private:
    KvsInstanceStateRegistry() noexcept = default;
};

/// @brief Shared in-memory state for a KVS backing store.
///
/// Handles opened for the same filename prefix share this state, including the
/// snapshot and storage limits. The storage limit applies to this store's files,
/// not to storage used by other KVS instances.
struct KvsInstanceState final
{
    KvsInstanceState() = default;

    KvsInstanceState(KvsInstanceState&&) = delete;
    KvsInstanceState(const KvsInstanceState&) = delete;
    KvsInstanceState& operator=(KvsInstanceState&&) = delete;
    KvsInstanceState& operator=(const KvsInstanceState&) = delete;

    /// Protects access to the shared state.
    std::mutex mutex;
    /// Values in the current snapshot.
    KvsMap current_values{};
    /// Default values loaded for this store.
    KvsMap default_values{};
    /// Instance ID that has been specified during creation.
    std::size_t instance_id{0UL};
    /// Snapshot ID that has been specified during creation.
    std::size_t snapshot_id{0UL};
    /// Maximum number of snapshots maintained by this instance.
    /// @note comp_req__kvs__snapshot_max_num_cfg
    std::size_t max_snapshots{0UL};
    /// Optional total storage limit for this store's files.
    std::optional<size_t> max_storage_bytes{std::nullopt};
    /// Prefix used to construct this store's file paths.
    score::filesystem::Path filename_prefix{};
};

/// @brief Compares the parameters that identify and configure the store.
/// @note Runtime values (current/default maps) and the mutex are not compared.
bool operator==(const KvsInstanceState& v1, const KvsInstanceState& v2) noexcept;
bool operator!=(const KvsInstanceState& v1, const KvsInstanceState& v2) noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_KVS_INSTANCE_STATE_H

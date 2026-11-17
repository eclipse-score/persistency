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
#include "score/kvs/kvs_parameters.h"
#include "score/kvs/kvs_value.h"

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>

namespace score::kvs
{
struct KvsInstanceState;

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

    /// Whether `Initialize()` has succeeded.
    bool initialized{false};

  private:
    KvsInstanceStateRegistry() noexcept = default;
};

/// @brief In-memory state shared by all `Kvs` handles of one instance ID.
///
/// Created by `Initialize()` from the configured options of the ID.
struct KvsInstanceState final
{
    /// @brief Creates an empty state for the given options.
    /// @param options Options of the instance.
    explicit KvsInstanceState(const KvsParameters& options);

    KvsInstanceState(KvsInstanceState&&) = delete;
    KvsInstanceState(const KvsInstanceState&) = delete;
    KvsInstanceState& operator=(KvsInstanceState&&) = delete;
    KvsInstanceState& operator=(const KvsInstanceState&) = delete;

    /// Protects access to the shared state.
    std::mutex mutex;
    /// Values in the current snapshot.
    KvsMap current_values;
    /// Default values loaded for this store.
    KvsMap default_values;
    /// Parameters that have been used to open this instance.
    KvsParameters params;
    /// Prefix used to construct this store's file paths.
    score::filesystem::Path filename_prefix;
};

/// @brief Loads the files of one instance and creates its state.
///
/// Takes no lock, does not check the instance ID range and does not touch the registry.
/// @param params Options of the instance.
/// @return The new state on success, or the ErrorCode of the failed file load.
score::Result<std::shared_ptr<KvsInstanceState>> OpenInstanceState(const KvsParameters& params) noexcept;

/// @brief Compares the options of two instances.
bool operator==(const KvsParameters& v1, const KvsParameters& v2) noexcept;
bool operator!=(const KvsParameters& v1, const KvsParameters& v2) noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_KVS_INSTANCE_STATE_H

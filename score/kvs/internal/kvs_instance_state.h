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
#include "score/kvs/kvs_builder.h"
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

  private:
    KvsInstanceStateRegistry() noexcept = default;
};

/// @brief In-memory state shared by all `Kvs` handles of one instance ID.
///
/// Created by the first `KvsBuilder::Build()` for the ID. Later builds must pass equal `params`.
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
    KvsMap current_values;
    /// Default values loaded for this store.
    KvsMap default_values;
    /// Parameters that have been used to open this instance.
    KvsOpenParameters params;
    /// Prefix used to construct this store's file paths.
    score::filesystem::Path filename_prefix;
};

/// @brief Compares the kvs open options.
bool operator==(const KvsOpenParameters& v1, const KvsOpenParameters& v2) noexcept;
bool operator!=(const KvsOpenParameters& v1, const KvsOpenParameters& v2) noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_KVS_INSTANCE_STATE_H

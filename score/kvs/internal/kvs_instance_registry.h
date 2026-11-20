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
#ifndef SCORE_KVS_INTERNAL_KVS_INSTANCE_REGISTRY_H
#define SCORE_KVS_INTERNAL_KVS_INSTANCE_REGISTRY_H

#include "score/kvs/kvs_builder.h"

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>

namespace score::kvs
{
/// @brief Registry of shared KVS instances, indexed by instance ID.
struct KvsInstanceRegistry final
{
  public:
    using Entry = std::shared_ptr<Kvs>;
    using Array = std::array<Entry, kMaxInstanceCount>;

    KvsInstanceRegistry(const KvsInstanceRegistry&) = delete;
    KvsInstanceRegistry(KvsInstanceRegistry&&) = delete;

    KvsInstanceRegistry& operator=(const KvsInstanceRegistry&) = delete;
    KvsInstanceRegistry& operator=(KvsInstanceRegistry&&) = delete;

    /// @brief Returns the process-wide registry instance.
    static KvsInstanceRegistry& GetInstance();

    /// @brief Removes the instance registered under the given ID.
    [[nodiscard]] score::Result<void> Unregister(std::size_t instance_id) noexcept;

    /// Serializes builds and unregistrations, and protects `array`.
    std::mutex mutex;

    /// Registered instances, indexed by instance ID.
    Array array;

  private:
    KvsInstanceRegistry() noexcept = default;
};

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_KVS_INSTANCE_REGISTRY_H

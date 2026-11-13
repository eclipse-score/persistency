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

#include "score/kvs/internal/kvs_instance_registry.h"

#include "score/kvs/internal/logger.h"

#include <utility>

namespace score::kvs
{

KvsInstanceRegistry& KvsInstanceRegistry::GetInstance()
{
    // codeql::misra_deviation_next_line(kvs-instance-registry)
    static KvsInstanceRegistry instance;
    return instance;
}

score::Result<void> KvsInstanceRegistry::Unregister(const std::size_t instance_id) noexcept
{
    Entry removed;
    {
        std::lock_guard<std::mutex> lock{mutex};
        if ((instance_id >= array.size()) || (array.at(instance_id) == nullptr))
        {
            GetLogger().LogWarn() << "[instance=" << instance_id << "]" << "Instance is not registered.";
            return score::MakeUnexpected(ErrorCode::kInvalidInstanceId);
        }

        removed = std::move(array.at(instance_id));
    }
    return {};
}

void KvsInstanceRegistry::Reset()
{
    std::unique_lock<std::mutex> lock(mutex);
    for (auto& entry : array)
    {
        entry = nullptr;
    }
}

}  // namespace score::kvs

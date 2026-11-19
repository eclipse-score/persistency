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

#include "score/kvs/kvs_runtime.h"

#include "score/kvs/internal/kvs_instance_state.h"
#include "score/kvs/internal/logger.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <string_view>
#include <utility>

namespace score::kvs
{

namespace
{

score::Result<void> InitializeFrom(const score::Result<KvsConfiguration>& configuration) noexcept
{
    if (!configuration.has_value())
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*configuration.error()));
    }
    return Initialize(configuration.value());
}

}  // namespace

score::Result<void> Initialize(const KvsConfiguration& configuration) noexcept
{
    auto& registry{KvsInstanceStateRegistry::GetInstance()};
    const std::unique_lock<std::mutex> lock{registry.mutex};

    if (registry.initialized)
    {
        GetLogger().LogError() << "KVS is already initialized.";
        return score::MakeUnexpected(ErrorCode::kAlreadyInitialized);
    }

    // The states stay local until every instance has opened, so a failure leaves the registry untouched.
    KvsInstanceStateRegistry::Array states;
    for (std::size_t id{0U}; id < kMaxInstanceCount; ++id)
    {
        const auto params{configuration.Find(InstanceId{id})};
        if (!params.has_value())
        {
            continue;
        }

        auto state{OpenInstanceState(*params)};
        if (!state)
        {
            return score::MakeUnexpected(static_cast<ErrorCode>(*state.error()));
        }
        states.at(id) = std::move(*state);
    }

    for (std::size_t id{0U}; id < kMaxInstanceCount; ++id)
    {
        if (states.at(id) != nullptr)
        {
            GetLogger().LogInfo() << states.at(id)->params << "KVS opened successfully.";
            registry.array.at(id) = std::move(states.at(id));
        }
    }
    registry.initialized = true;

    return {};
}

score::Result<void> Initialize(const std::string_view path) noexcept
{
    return InitializeFrom(KvsConfiguration::Load(path));
}

score::Result<void> Initialize() noexcept
{
    return InitializeFrom(KvsConfiguration::Load());
}

score::Result<Kvs> Get(const InstanceId& instance_id) noexcept
{
    if (instance_id.id >= kMaxInstanceCount)
    {
        GetLogger().LogError() << "[instance=" << instance_id.id << "]"
                               << "Instance ID out of range, must be below" << kMaxInstanceCount;
        return score::MakeUnexpected(ErrorCode::kInvalidInstanceId);
    }

    auto& registry{KvsInstanceStateRegistry::GetInstance()};
    const std::unique_lock<std::mutex> lock{registry.mutex};

    const auto& entry{registry.array.at(instance_id.id)};
    if (entry == nullptr)
    {
        GetLogger().LogError() << "[instance=" << instance_id.id << "]" << "Instance is not initialized.";
        return score::MakeUnexpected(ErrorCode::kInstanceNotInitialized);
    }

    Kvs kvs;
    kvs.state_ = entry;
    return kvs;
}

}  // namespace score::kvs

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
#include "score/kvs/kvs_configuration.h"

#include "score/kvs/internal/config_parser.h"
#include "score/kvs/internal/logger.h"

namespace score::kvs
{

namespace
{

score::Result<void> Validate(const KvsParameters& options) noexcept
{
    if (options.instance_id >= kMaxInstanceCount)
    {
        GetLogger().LogError() << "[instance=" << options.instance_id << "]"
                               << "Instance ID out of range, must be below" << kMaxInstanceCount;
        return score::MakeUnexpected(ErrorCode::kInvalidConfiguration);
    }

    if (options.snapshot_count_limit > kMaxSnapshotsUpperBound)
    {
        GetLogger().LogError() << "[instance=" << options.instance_id << "]"
                               << "Snapshot count limit" << options.snapshot_count_limit << "exceeds the upper bound"
                               << kMaxSnapshotsUpperBound;
        return score::MakeUnexpected(ErrorCode::kInvalidConfiguration);
    }

    if (options.storage_quota.has_value() && (*options.storage_quota == 0U))
    {
        GetLogger().LogError() << "[instance=" << options.instance_id << "]" << "Storage quota must not be 0";
        return score::MakeUnexpected(ErrorCode::kInvalidConfiguration);
    }

    return {};
}

}  // namespace

score::Result<KvsConfiguration> KvsConfiguration::Load(const std::string_view path) noexcept
{
    return Parse(path);
}

score::Result<KvsConfiguration> KvsConfiguration::Load() noexcept
{
    constexpr std::string_view kDefaultConfigPath{"./etc/kvs_config.json"};
    return Load(kDefaultConfigPath);
}

score::Result<void> KvsConfiguration::Add(const KvsParameters& options) noexcept
{
    const auto valid{Validate(options)};
    if (!valid.has_value())
    {
        return valid;
    }

    auto& entry{entries_.at(options.instance_id)};
    if (entry.has_value())
    {
        GetLogger().LogError() << "[instance=" << options.instance_id << "]" << "Instance already configured";
        return score::MakeUnexpected(ErrorCode::kInstanceAlreadyConfigured);
    }

    entry = options;
    return {};
}

score::cpp::optional<KvsParameters> KvsConfiguration::Find(const InstanceId& instance_id) const noexcept
{
    if (instance_id.id >= entries_.size())
    {
        return score::cpp::nullopt;
    }
    return entries_.at(instance_id.id);
}

}  // namespace score::kvs

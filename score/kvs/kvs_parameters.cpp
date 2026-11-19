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

#include "score/kvs/kvs_parameters.h"

#include "score/kvs/internal/logger.h"

#include <algorithm>
#include <string>
#include <type_traits>

namespace score::kvs
{

namespace
{

template <typename LoadPolicy,
          std::enable_if_t<std::is_same_v<LoadPolicy, KvsDefaults> || std::is_same_v<LoadPolicy, KvsLoad>, bool> = true>
score::mw::log::LogStream& operator<<(score::mw::log::LogStream& stream, const LoadPolicy& policy) noexcept
{
    switch (policy)
    {
        case LoadPolicy::kRequired:
            return stream << "kRequired";
        case LoadPolicy::kOptional:
            return stream << "kOptional";
        case LoadPolicy::kIgnored:
            return stream << "kIgnored";
        default:
            return stream << "unknown";
    }
}

}  // namespace

score::mw::log::LogStream& operator<<(score::mw::log::LogStream& stream, const KvsParameters& params)
{
    stream << "[instance_id=" << params.instance_id << "]";
    stream << "[snapshot_id=" << params.snapshot_id << "]";
    stream << "[current_load_policy=" << params.current_load_policy << "]";
    stream << "[default_load_policy=" << params.default_load_policy << "]";
    stream << "[working_dir=" << params.working_dir << "]";
    stream << "[storage_quota=" << (params.storage_quota ? std::to_string(*(params.storage_quota)) : "unlimited")
           << "]";
    stream << "[snapshot_count_limit=" << params.snapshot_count_limit << "]";
    return stream;
}

KvsParameters::KvsParameters(const InstanceId& instance_id) noexcept : instance_id{instance_id.id} {}

KvsParametersBuilder::KvsParametersBuilder(const InstanceId& instance_id) noexcept : params_{instance_id} {}

KvsParametersBuilder& KvsParametersBuilder::Snapshot(const SnapshotId& snapshot_id) noexcept
{
    params_.snapshot_id = snapshot_id.id;
    return *this;
}

KvsParametersBuilder& KvsParametersBuilder::Defaults(const KvsDefaults& mode) noexcept
{
    params_.default_load_policy = mode;
    return *this;
}

KvsParametersBuilder& KvsParametersBuilder::KvsLoad(const score::kvs::KvsLoad& mode) noexcept
{
    params_.current_load_policy = mode;
    return *this;
}

KvsParametersBuilder& KvsParametersBuilder::WorkingDir(const std::string& working_dir) noexcept
{
    params_.working_dir = working_dir;
    return *this;
}

KvsParametersBuilder& KvsParametersBuilder::MaxStorageBytes(const std::optional<size_t>& max_storage_bytes) noexcept
{
    params_.storage_quota = max_storage_bytes;
    return *this;
}

KvsParametersBuilder& KvsParametersBuilder::SnapshotMaxCount(const std::size_t max_count) noexcept
{
    const std::size_t max_snapshots_clamped{std::min(max_count, kMaxSnapshotsUpperBound)};

    if (max_snapshots_clamped != max_count)
    {
        GetLogger().LogWarn() << "[instance=" << params_.instance_id << "]" << "SnapshotMaxCount" << max_count
                              << "exceeds the upper bound, clamped to" << kMaxSnapshotsUpperBound;
    }

    params_.snapshot_count_limit = max_snapshots_clamped;

    return *this;
}

KvsParameters KvsParametersBuilder::Build() const noexcept
{
    return params_;
}

}  // namespace score::kvs

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

#include "score/kvs/kvs_builder.h"

#include "score/kvs/internal/kvs_constants.h"
#include "score/kvs/internal/kvs_instance_registry.h"
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

score::mw::log::LogStream& operator<<(score::mw::log::LogStream& stream, const KvsOpenParameters& params)
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

KvsBuilder::KvsBuilder(const InstanceId& instance_id) noexcept
{
    params_.instance_id = instance_id.id;
    params_.snapshot_id = 0UL;
    params_.default_load_policy = KvsDefaults::kOptional;
    params_.current_load_policy = KvsLoad::kOptional;
    params_.snapshot_count_limit = kDefaultMaxSnapshots;
    params_.working_dir = "./data_folder/";
    params_.storage_quota = std::nullopt;
}

KvsBuilder& KvsBuilder::Snapshot(const SnapshotId& snapshot_id) noexcept
{
    params_.snapshot_id = snapshot_id.id;
    return *this;
}

KvsBuilder& KvsBuilder::Defaults(const KvsDefaults& mode) noexcept
{
    params_.default_load_policy = mode;
    return *this;
}

KvsBuilder& KvsBuilder::KvsLoad(const score::kvs::KvsLoad& mode) noexcept
{
    params_.current_load_policy = mode;
    return *this;
}

KvsBuilder& KvsBuilder::WorkingDir(const std::string& working_dir) noexcept
{
    params_.working_dir = working_dir;
    return *this;
}

KvsBuilder& KvsBuilder::MaxStorageBytes(const std::optional<size_t>& max_storage_bytes) noexcept
{
    params_.storage_quota = max_storage_bytes;
    return *this;
}

KvsBuilder& KvsBuilder::SnapshotMaxCount(const std::size_t max_count) noexcept
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

score::Result<std::shared_ptr<Kvs>> KvsBuilder::Build() const noexcept
{
    // Lock the registry.
    std::unique_lock<std::mutex> lock{KvsInstanceRegistry::GetInstance().mutex};

    // Out of range.
    if (params_.instance_id >= KvsInstanceRegistry::GetInstance().array.size())
    {
        GetLogger().LogError() << "[instance=" << params_.instance_id << "]"
                               << "Instance ID out of range, must be below" << kMaxInstanceCount;
        return score::MakeUnexpected(ErrorCode::kInvalidInstanceId);
    }

    // Check if already exists.
    auto& entry{KvsInstanceRegistry::GetInstance().array.at(params_.instance_id)};
    if (entry)
    {
        // Has it been created with different params?
        if (entry->params_ != params_)
        {
            GetLogger().LogError() << "[instance=" << params_.instance_id << "]"
                                   << "Instance already open with different parameters. Existing:" << entry->params_
                                   << "Requested:" << params_;
            return score::MakeUnexpected(ErrorCode::kInstanceParametersMismatch);
        }

        return entry;
    }

    Kvs kvs;

    const std::string directory{params_.working_dir.empty() ? std::string{"./"} : params_.working_dir};
    const score::filesystem::Path base_path{directory};
    score::filesystem::Path filename_prefix{base_path /
                                            (std::string{kFilenamePrefix} + std::to_string(params_.instance_id))};
    const score::filesystem::Path filename_default{filename_prefix.Native() + std::string{kDefaultSuffix}};

    score::Result<KvsMap> default_values{};
    // Load the default values if requested.
    if (params_.default_load_policy != KvsDefaults::kIgnored)
    {
        const OpenJsonNeedFile need_file{(params_.default_load_policy == KvsDefaults::kRequired)
                                             ? OpenJsonNeedFile::kRequired
                                             : OpenJsonNeedFile::kOptional};
        default_values = kvs.OpenJson(filename_default, need_file);
        if (!default_values)
        {
            return score::MakeUnexpected(static_cast<ErrorCode>(*default_values.error()));
        }
    }

    const score::filesystem::Path filename_kvs{filename_prefix.Native() + std::string{kSnapshotSeparator} +
                                               std::to_string(params_.snapshot_id)};

    score::Result<KvsMap> current_values{};
    // Load the current values if requested.
    if (params_.current_load_policy != KvsLoad::kIgnored)
    {
        const OpenJsonNeedFile need_file{(params_.current_load_policy == KvsLoad::kRequired)
                                             ? OpenJsonNeedFile::kRequired
                                             : OpenJsonNeedFile::kOptional};
        current_values = kvs.OpenJson(filename_kvs, need_file);
        if (!current_values)
        {
            return score::MakeUnexpected(static_cast<ErrorCode>(*current_values.error()));
        }
    }

    kvs.filename_prefix_ = std::move(filename_prefix);
    kvs.current_values_ = std::move(*current_values);
    kvs.default_values_ = std::move(*default_values);
    kvs.params_ = params_;

    entry = std::make_shared<Kvs>(std::move(kvs));

    GetLogger().LogInfo() << entry->params_ << "KVS opened successfully.";

    return entry;
}

score::Result<void> Unregister(const InstanceId& instance_id) noexcept
{
    return KvsInstanceRegistry::GetInstance().Unregister(instance_id.id);
}

}  // namespace score::kvs

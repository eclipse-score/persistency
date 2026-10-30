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

#include "score/kvs/internal/kvs_instance_state.hpp"

namespace score::kvs
{

KvsBuilder::KvsBuilder(const InstanceId& instance_id) noexcept
    : instance_id_{instance_id},
      snapshot_id_{SnapshotId{0}},
      defaults_{KvsDefaults::kOptional},
      kvs_load_{score::kvs::KvsLoad::kOptional},
      working_dir_{"./data_folder/"},
      maximum_storage_bytes_{std::nullopt},
      max_snapshots_{kDefaultMaxSnapshots}
{
}

KvsBuilder& KvsBuilder::Snapshot(const SnapshotId& snapshot_id) noexcept
{
    this->snapshot_id_ = snapshot_id;
    return *this;
}

KvsBuilder& KvsBuilder::Defaults(const KvsDefaults& mode) noexcept
{
    defaults_ = mode;
    return *this;
}

KvsBuilder& KvsBuilder::KvsLoad(const score::kvs::KvsLoad& mode) noexcept
{
    kvs_load_ = mode;
    return *this;
}

KvsBuilder& KvsBuilder::WorkingDir(const std::string& working_dir) noexcept
{
    this->working_dir_ = working_dir;
    return *this;
}

KvsBuilder& KvsBuilder::MaxStorageBytes(const std::optional<size_t>& max_storage_bytes) noexcept
{
    this->maximum_storage_bytes_ = max_storage_bytes;
    return *this;
}

KvsBuilder& KvsBuilder::SnapshotMaxCount(const std::size_t max_count) noexcept
{
    this->max_snapshots_ = max_count;
    return *this;
}

score::Result<Kvs> KvsBuilder::Build() const noexcept
{
    // Lock the registry.
    std::unique_lock<std::mutex> lock{KvsInstanceStateRegistry::GetInstance().mutex};

    // Out of range.
    if (instance_id_.id >= KvsInstanceStateRegistry::GetInstance().array.size())
    {
        return score::MakeUnexpected(ErrorCode::kResourceBusy);
    }

    const std::size_t max_snapshots_clamped{std::clamp(max_snapshots_, 0UL, kMaxSnapshotsUpperBound)};

    Kvs kvs;

    // Check if already exists.
    auto& entry{KvsInstanceStateRegistry::GetInstance().array.at(instance_id_.id)};
    if (entry && (entry->instance_id == instance_id_.id))
    {
        // Has it been created with different params?
        if ((entry->snapshot_id != snapshot_id_.id) || (entry->max_snapshots != max_snapshots_clamped) ||
            (entry->max_storage_bytes != maximum_storage_bytes_))
        {
            return score::MakeUnexpected(ErrorCode::kInstanceParametersMismatch);
        }

        kvs.state_ = entry;
        return std::move(kvs);
    }

    const std::string directory{working_dir_.empty() ? std::string{"./"} : working_dir_};
    const score::filesystem::Path base_path{directory};
    score::filesystem::Path filename_prefix{base_path / ("kvs_" + std::to_string(instance_id_.id))};

    const score::filesystem::Path filename_default{filename_prefix.Native() + "_default"};
    score::Result<KvsMap> default_values{};

    // Load the current values if requsted.
    if (defaults_ != KvsDefaults::kIgnored)
    {
        const OpenJsonNeedFile need_file{(defaults_ == KvsDefaults::kRequired) ? OpenJsonNeedFile::kRequired
                                                                               : OpenJsonNeedFile::kOptional};
        default_values = kvs.OpenJson(filename_default, need_file);
        if (!default_values)
        {
            return score::MakeUnexpected(static_cast<ErrorCode>(*default_values.error()));
        }
    }

    const score::filesystem::Path filename_kvs{filename_prefix.Native() + "_" + std::to_string(snapshot_id_.id)};
    score::Result<KvsMap> current_values{};

    // Load the default values if requested.
    if (kvs_load_ != KvsLoad::kIgnored)
    {
        const OpenJsonNeedFile need_file{(kvs_load_ == KvsLoad::kRequired) ? OpenJsonNeedFile::kRequired
                                                                           : OpenJsonNeedFile::kOptional};
        current_values = kvs.OpenJson(filename_kvs, need_file);
        if (!current_values)
        {
            return score::MakeUnexpected(static_cast<ErrorCode>(*current_values.error()));
        }
    }

    entry = std::make_shared<KvsInstanceState>();

    entry->instance_id = instance_id_.id;
    entry->snapshot_id = snapshot_id_.id;
    entry->filename_prefix = std::move(filename_prefix);
    entry->current_values = std::move(current_values.value());
    entry->default_values = std::move(default_values.value());
    entry->max_snapshots = max_snapshots_clamped;
    entry->max_storage_bytes = maximum_storage_bytes_;

    kvs.logger_->LogInfo() << "[instance=" << entry->instance_id << "]" << "[snapshot=" << entry->snapshot_id << "]"
                           << "[maxSnapshotCount=" << entry->max_snapshots << "]"
                           << "[maxStorageBytes=" << entry->max_storage_bytes.value_or(0UL) << "]" << " KVS created";

    kvs.state_ = entry;

    return std::move(kvs);
}

}  // namespace score::kvs

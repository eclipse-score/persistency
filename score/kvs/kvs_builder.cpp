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

namespace score::kvs
{

KvsBuilder::KvsBuilder(const InstanceId& instance_id) noexcept
    : instance_id_(instance_id),
      snapshot_id_(SnapshotId{0}),
      need_defaults_(OpenNeedDefaults::kOptional),
      need_kvs_(OpenNeedKvs::kOptional),
      directory_("./data_folder/"),
      maximum_storage_bytes_(std::nullopt),
      max_snapshots_(kDefaultMaxSnapshots)
{
}

KvsBuilder& KvsBuilder::Snapshot(const SnapshotId& snapshot_id) noexcept
{
    this->snapshot_id_ = snapshot_id;
    return *this;
}

KvsBuilder& KvsBuilder::NeedDefaultsFlag(const OpenNeedDefaults& flag) noexcept
{
    need_defaults_ = flag;
    return *this;
}

KvsBuilder& KvsBuilder::NeedKvsFlag(const OpenNeedKvs& flag) noexcept
{
    need_kvs_ = flag;
    return *this;
}

KvsBuilder& KvsBuilder::Dir(const std::string& dir_path) noexcept
{
    this->directory_ = dir_path;
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
    // Use current directory if empty.
    const std::string directory = directory_.empty() ? std::string{"./"} : directory_;

    return Kvs::Open(
        instance_id_, snapshot_id_, need_defaults_, need_kvs_, directory, maximum_storage_bytes_, max_snapshots_);
}

}  // namespace score::kvs

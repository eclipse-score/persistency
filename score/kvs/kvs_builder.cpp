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
      defaults_(KvsDefaults::kOptional),
      kvs_load_(score::kvs::KvsLoad::kOptional),
      working_dir_("./data_folder/"),
      maximum_storage_bytes_(std::nullopt),
      max_snapshots_(kDefaultMaxSnapshots)
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
    // Use current directory if empty.
    const std::string working_dir = working_dir_.empty() ? std::string{"./"} : working_dir_;

    return Kvs::Open(
        instance_id_, snapshot_id_, defaults_, kvs_load_, working_dir, maximum_storage_bytes_, max_snapshots_);
}

}  // namespace score::kvs

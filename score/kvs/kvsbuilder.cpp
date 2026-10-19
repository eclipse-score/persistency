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
#include "score/kvs/kvsbuilder.hpp"

namespace score::mw::per::kvs
{

/*********************** KVS Builder Implementation *********************/
KvsBuilder::KvsBuilder(const InstanceId& instance_id) noexcept
    : instance_id_(instance_id),
      snapshot_id_(SnapshotId{0}),
      need_defaults_(OpenNeedDefaults::Optional),
      need_kvs_(OpenNeedKvs::Optional),
      directory_("./data_folder/"),         /* Default Directory */
      maximum_storage_bytes_(std::nullopt), /* No storage limit by default */
      max_snapshots_(KVS_DEFAULT_MAX_SNAPSHOTS)
{
}

KvsBuilder& KvsBuilder::snapshot(const SnapshotId& snapshot_id) noexcept
{
    this->snapshot_id_ = snapshot_id;
    return *this;
}

KvsBuilder& KvsBuilder::need_defaults_flag(const OpenNeedDefaults& flag) noexcept
{
    need_defaults_ = flag;
    return *this;
}

KvsBuilder& KvsBuilder::need_kvs_flag(const OpenNeedKvs& flag) noexcept
{
    need_kvs_ = flag;
    return *this;
}

KvsBuilder& KvsBuilder::dir(std::string&& dir_path) noexcept
{
    this->directory_ = std::move(dir_path);
    return *this;
}

KvsBuilder& KvsBuilder::max_storage_bytes(const std::optional<size_t>& max_storage_bytes) noexcept
{
    this->maximum_storage_bytes_ = max_storage_bytes;
    return *this;
}

KvsBuilder& KvsBuilder::snapshot_max_count(std::size_t max_count) noexcept
{
    this->max_snapshots_ = max_count;
    return *this;
}

score::Result<Kvs> KvsBuilder::build() noexcept
{
    score::Result<Kvs> result = score::MakeUnexpected(ErrorCode::UnmappedError);

    /* Use current directory if empty */
    if ("" == directory_)
    {
        directory_ = "./";
    }

    result = Kvs::open(instance_id_,
                       snapshot_id_,
                       need_defaults_,
                       need_kvs_,
                       std::move(directory_),
                       maximum_storage_bytes_,
                       max_snapshots_);

    return result;
}

} /* namespace score::mw::per::kvs */

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
KvsBuilder::KvsBuilder(const InstanceId& instance_id)
    : instance_id(instance_id),
      snapshot_id(SnapshotId{0}),
      need_defaults(OpenNeedDefaults::Optional),
      need_kvs(OpenNeedKvs::Optional),
      directory("./data_folder/") /* Default Directory */
{
}

KvsBuilder& KvsBuilder::snapshot(const SnapshotId& snapshot_id)
{
    this->snapshot_id = snapshot_id;
    return *this;
}

KvsBuilder& KvsBuilder::need_defaults_flag(const OpenNeedDefaults& flag)
{
    need_defaults = flag;
    return *this;
}

KvsBuilder& KvsBuilder::need_kvs_flag(const OpenNeedKvs& flag)
{
    need_kvs = flag;
    return *this;
}

KvsBuilder& KvsBuilder::dir(std::string&& dir_path)
{
    this->directory = std::move(dir_path);
    return *this;
}

score::Result<Kvs> KvsBuilder::build()
{
    score::Result<Kvs> result = score::MakeUnexpected(ErrorCode::UnmappedError);

    /* Use current directory if empty */
    if ("" == directory)
    {
        directory = "./";
    }

    result = Kvs::open(instance_id,
                       snapshot_id,
                       need_defaults,
                       need_kvs,
                       std::move(directory));

    return result;
}

} /* namespace score::mw::per::kvs */

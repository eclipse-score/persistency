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
#include "kvsbuilder.hpp"

namespace score::mw::per::kvs
{

/*********************** KVS Builder Implementation *********************/
KvsBuilder::KvsBuilder(const InstanceId& instance_id)
    : instance_id(instance_id),
      need_defaults(false),
      need_kvs(false),
      directory("./data_folder/"),                 /* Default Directory */
      maximum_size(KVS_DEFAULT_MAX_SIZE_BYTES)     /* Default max size in bytes */
{
}

KvsBuilder& KvsBuilder::need_defaults_flag(bool flag)
{
    need_defaults = flag;
    return *this;
}

KvsBuilder& KvsBuilder::need_kvs_flag(bool flag)
{
    need_kvs = flag;
    return *this;
}

KvsBuilder& KvsBuilder::dir(std::string&& dir_path)
{
    this->directory = std::move(dir_path);
    return *this;
}

KvsBuilder& KvsBuilder::max_size(std::optional<size_t> max_bytes)
{
    this->maximum_size = max_bytes;
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
                       need_defaults ? OpenNeedDefaults::Required : OpenNeedDefaults::Optional,
                       need_kvs ? OpenNeedKvs::Required : OpenNeedKvs::Optional,
                       std::move(directory),
                       maximum_size);

    return result;
}

} /* namespace score::mw::per::kvs */

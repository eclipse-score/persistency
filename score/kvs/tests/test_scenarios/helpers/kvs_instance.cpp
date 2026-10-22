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

#include "helpers/kvs_instance.h"

#include "score/kvs/kvs_builder.h"

using namespace score::kvs;

Kvs KvsInstance(const KvsParameters& params)
{
    InstanceId instance_id{params.instance_id};
    KvsBuilder builder{instance_id};

    // Set parameters.
    if (params.need_defaults.has_value())
    {
        builder = builder.NeedDefaultsFlag(*params.need_defaults);
    }
    if (params.need_kvs.has_value())
    {
        builder = builder.NeedKvsFlag(*params.need_kvs);
    }
    if (params.dir.has_value())
    {
        builder = builder.Dir(std::string(*params.dir));
    }
    if (params.snapshot_max_count.has_value())
    {
        builder = builder.SnapshotMaxCount(*params.snapshot_max_count);
    }

    // Build KVS instance.
    auto build_result{builder.Build()};
    if (build_result)
    {
        return std::move(*build_result);
    }
    else
    {
        throw std::runtime_error{"Failed to build KVS instance"};
    }
}

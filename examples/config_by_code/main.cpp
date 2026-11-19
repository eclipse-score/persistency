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
#include "score/kvs/kvs_parameters.h"
#include "score/kvs/kvs_runtime.h"

#include <cstdlib>
#include <iostream>

using namespace score::kvs;

// Configures one instance in code and opens it.
int main()
{
    KvsConfiguration configuration;
    if (!configuration.Add(KvsParametersBuilder{InstanceId{0U}}.SnapshotMaxCount(2U).Build()).has_value())
    {
        std::cerr << "Failed to add instance 0\n";
        return EXIT_FAILURE;
    }

    if (!Initialize(configuration).has_value())
    {
        std::cerr << "Failed to initialize the KVS\n";
        return EXIT_FAILURE;
    }

    const score::Result<Kvs> kvs{Get(InstanceId{0U})};
    if (!kvs.has_value())
    {
        std::cerr << "Failed to get instance 0\n";
        return EXIT_FAILURE;
    }

    std::cout << "Instance 0 is ready\n";
    return EXIT_SUCCESS;
}

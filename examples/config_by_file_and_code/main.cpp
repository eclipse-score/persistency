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

// Configures instances 0 and 1 from the file given as the first argument, adds instance 2 in code and opens it.
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <kvs_config.json>\n";
        return EXIT_FAILURE;
    }

    score::Result<KvsConfiguration> configuration{KvsConfiguration::Load(argv[1])};
    if (!configuration.has_value())
    {
        std::cerr << "Failed to load " << argv[1] << '\n';
        return EXIT_FAILURE;
    }

    // Only instances absent from the file can be added.
    // Adding instance 0 or 1 fails with kInstanceAlreadyConfigured.
    const KvsParameters parameters{KvsParametersBuilder{InstanceId{2U}}.Defaults(KvsDefaults::kIgnored).Build()};
    if (!configuration.value().Add(parameters).has_value())
    {
        std::cerr << "Failed to add instance 2\n";
        return EXIT_FAILURE;
    }

    if (!Initialize(configuration.value()).has_value())
    {
        std::cerr << "Failed to initialize the KVS\n";
        return EXIT_FAILURE;
    }

    const score::Result<Kvs> kvs{Get(InstanceId{2U})};
    if (!kvs.has_value())
    {
        std::cerr << "Failed to get instance 2\n";
        return EXIT_FAILURE;
    }

    std::cout << "Instance 2 is ready\n";
    return EXIT_SUCCESS;
}

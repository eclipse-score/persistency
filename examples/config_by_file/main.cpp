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
#include "score/kvs/kvs_runtime.h"

#include <cstdlib>
#include <iostream>

using namespace score::kvs;

// Configures instances 0 and 1 from the file given as the first argument and opens them.
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <kvs_config.json>\n";
        return EXIT_FAILURE;
    }

    if (!Initialize(argv[1]).has_value())
    {
        std::cerr << "Failed to initialize the KVS from " << argv[1] << '\n';
        return EXIT_FAILURE;
    }

    for (const InstanceId instance_id : {InstanceId{0U}, InstanceId{1U}})
    {
        if (!Get(instance_id).has_value())
        {
            std::cerr << "Failed to get instance " << instance_id.id << '\n';
            return EXIT_FAILURE;
        }
        std::cout << "Instance " << instance_id.id << " is ready\n";
    }
    return EXIT_SUCCESS;
}

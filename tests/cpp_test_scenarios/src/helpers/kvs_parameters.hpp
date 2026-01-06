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

#pragma once

#include <kvs.hpp>

struct KvsParameters
{
    uint64_t instance_id;
    std::optional<bool> need_defaults;
    std::optional<bool> need_kvs;
    std::optional<std::string> dir;
};

KvsParameters map_to_params(const std::string &data);

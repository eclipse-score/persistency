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

#include "score/kvs/internal/kvs_instance_state.hpp"

namespace score::kvs
{

KvsInstanceStateRegistry& KvsInstanceStateRegistry::GetInstance()
{
    static KvsInstanceStateRegistry instance;
    return instance;
}

void KvsInstanceStateRegistry::Reset()
{
    std::unique_lock<std::mutex> lock(mutex);
    for (auto& entry : array)
    {
        entry.reset();
    }
}

}  // namespace score::kvs

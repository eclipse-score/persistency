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

#include "score/kvs/internal/kvs_instance_state.h"

namespace score::kvs
{

KvsInstanceStateRegistry& KvsInstanceStateRegistry::GetInstance()
{
    // codeql::misra_deviation_next_line(kvs-instance-state-registry)
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

bool operator==(const KvsInstanceState& v1, const KvsInstanceState& v2) noexcept
{
    return (v1.instance_id == v2.instance_id) && (v1.snapshot_id == v2.snapshot_id) &&
           (v1.filename_prefix == v2.filename_prefix) && (v1.max_snapshots == v2.max_snapshots) &&
           (v1.max_storage_bytes == v2.max_storage_bytes);
}

bool operator!=(const KvsInstanceState& v1, const KvsInstanceState& v2) noexcept
{
    return !(v1 == v2);
}

}  // namespace score::kvs

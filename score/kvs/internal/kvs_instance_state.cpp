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

bool operator==(const KvsOpenParameters& v1, const KvsOpenParameters& v2) noexcept
{
    return (v1.current_load_policy == v2.current_load_policy) && (v1.default_load_policy == v2.default_load_policy) &&
           (v1.snapshot_id == v2.snapshot_id) && (v1.instance_id == v2.instance_id) &&
           (v1.snapshot_count_limit == v2.snapshot_count_limit) && (v1.storage_quota == v2.storage_quota) &&
           (v1.working_dir == v2.working_dir);
}

bool operator!=(const KvsOpenParameters& v1, const KvsOpenParameters& v2) noexcept
{
    return !(v1 == v2);
}

}  // namespace score::kvs

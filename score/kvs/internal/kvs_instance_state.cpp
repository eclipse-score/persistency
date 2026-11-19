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

#include "score/kvs/internal/kvs_constants.h"
#include "score/kvs/kvs.h"

#include <string>
#include <utility>

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
    initialized = false;
}

KvsInstanceState::KvsInstanceState(const KvsParameters& options) : params{options} {}

score::Result<std::shared_ptr<KvsInstanceState>> OpenInstanceState(const KvsParameters& params) noexcept
{
    const Kvs kvs;

    const std::string directory{params.working_dir.empty() ? std::string{"./"} : params.working_dir};
    const score::filesystem::Path base_path{directory};
    score::filesystem::Path filename_prefix{base_path /
                                            (std::string{kFilenamePrefix} + std::to_string(params.instance_id))};
    const score::filesystem::Path filename_default{filename_prefix.Native() + std::string{kDefaultSuffix}};

    score::Result<KvsMap> default_values{};
    // Load the default values if requested.
    if (params.default_load_policy != KvsDefaults::kIgnored)
    {
        const OpenJsonNeedFile need_file{(params.default_load_policy == KvsDefaults::kRequired)
                                             ? OpenJsonNeedFile::kRequired
                                             : OpenJsonNeedFile::kOptional};
        default_values = kvs.OpenJson(filename_default, need_file);
        if (!default_values)
        {
            return score::MakeUnexpected(static_cast<ErrorCode>(*default_values.error()));
        }
    }

    const score::filesystem::Path filename_kvs{filename_prefix.Native() + std::string{kSnapshotSeparator} +
                                               std::to_string(params.snapshot_id)};

    score::Result<KvsMap> current_values{};
    // Load the current values if requested.
    if (params.current_load_policy != KvsLoad::kIgnored)
    {
        const OpenJsonNeedFile need_file{(params.current_load_policy == KvsLoad::kRequired)
                                             ? OpenJsonNeedFile::kRequired
                                             : OpenJsonNeedFile::kOptional};
        current_values = kvs.OpenJson(filename_kvs, need_file);
        if (!current_values)
        {
            return score::MakeUnexpected(static_cast<ErrorCode>(*current_values.error()));
        }
    }

    auto state{std::make_shared<KvsInstanceState>(params)};
    state->filename_prefix = std::move(filename_prefix);
    state->current_values = std::move(*current_values);
    state->default_values = std::move(*default_values);
    return state;
}

bool operator==(const KvsParameters& v1, const KvsParameters& v2) noexcept
{
    return (v1.current_load_policy == v2.current_load_policy) && (v1.default_load_policy == v2.default_load_policy) &&
           (v1.snapshot_id == v2.snapshot_id) && (v1.instance_id == v2.instance_id) &&
           (v1.snapshot_count_limit == v2.snapshot_count_limit) && (v1.storage_quota == v2.storage_quota) &&
           (v1.working_dir == v2.working_dir);
}

bool operator!=(const KvsParameters& v1, const KvsParameters& v2) noexcept
{
    return !(v1 == v2);
}

}  // namespace score::kvs

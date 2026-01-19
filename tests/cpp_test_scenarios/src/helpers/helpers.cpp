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

#include "helpers/helpers.hpp"
#include <sstream>

std::pair<std::string, std::string> kvs_hash_paths(const std::string& working_dir,
                                                   score::mw::per::kvs::InstanceId instance_id,
                                                   score::mw::per::kvs::SnapshotId snapshot_id)
{
    std::ostringstream kvs_path, hash_path;
    kvs_path << working_dir << "/kvs_" << instance_id.id << "_" << snapshot_id.id << ".json";
    hash_path << working_dir << "/kvs_" << instance_id.id << "_" << snapshot_id.id << ".hash";

    return std::make_pair(kvs_path.str(), hash_path.str());
}

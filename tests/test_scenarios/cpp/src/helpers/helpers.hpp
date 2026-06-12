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
#include <string>
#include <utility>

std::pair<std::string, std::string> kvs_hash_paths(const std::string& working_dir,
                                                   score::mw::per::kvs::InstanceId instance_id,
                                                   score::mw::per::kvs::SnapshotId snapshot_id);

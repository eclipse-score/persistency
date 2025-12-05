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
#ifndef TEST_OBJECT_SNAPSHOT_HPP
#define TEST_OBJECT_SNAPSHOT_HPP

#include "../cit/test_snapshots.hpp"
#include <memory>

Scenario::Ptr snapshot_count_scenario = std::make_unique<SnapshotCount>();
Scenario::Ptr snapshot_count_max_count_scenario = std::make_unique<SnapshotMaxCount>();
Scenario::Ptr snapshot_count_restore_scenario = std::make_unique<SnapshotRestore>();
Scenario::Ptr snapshot_count_paths_scenario = std::make_unique<SnapshotPaths>();
ScenarioGroup::Ptr snapshot_group{new ScenarioGroupImpl{
    "snapshots",
{snapshot_count_scenario, snapshot_count_max_count_scenario, snapshot_count_restore_scenario,
    snapshot_count_paths_scenario},{}}};

#endif //TEST_OBJECT_SNAPSHOT_HPP
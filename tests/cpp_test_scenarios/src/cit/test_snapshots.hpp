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
#ifndef TEST_SNAPSHOT_HPP
#define TEST_SNAPSHOT_HPP

#include "scenario.hpp"

class SnapshotCount : public Scenario {
public:
  ~SnapshotCount() final = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

class SnapshotMaxCount : public Scenario {
public:
  ~SnapshotMaxCount() final = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

class SnapshotRestore : public Scenario {
public:
  ~SnapshotRestore() final = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

class SnapshotPaths : public Scenario {
public:
  ~SnapshotPaths() final = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

extern Scenario::Ptr snapshot_count_scenario;
extern Scenario::Ptr snapshot_count_max_count_scenario;
extern Scenario::Ptr snapshot_count_restore_scenario;
extern Scenario::Ptr snapshot_count_paths_scenario;
extern ScenarioGroup::Ptr snapshot_group;

#endif // TEST_SNAPSHOT_HPP
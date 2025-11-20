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

#include "scenario.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Each scenario is a class derived from Scenario, similar to Rust

class DefaultValuesScenario final : public Scenario {
public:
  ~DefaultValuesScenario() override = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

class RemoveKeyScenario final : public Scenario {
public:
  ~RemoveKeyScenario() override = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

class ResetAllKeysScenario final : public Scenario {
public:
  ~ResetAllKeysScenario() override = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

class ResetSingleKeyScenario final : public Scenario {
public:
  ~ResetSingleKeyScenario() override = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

class ChecksumScenario final : public Scenario {
public:
  ~ChecksumScenario() override = default;
  std::string name() const override;
  void run(const std::string &input) const override;
};

// Helper to get all scenarios
std::vector<std::shared_ptr<const Scenario>> get_default_value_scenarios();

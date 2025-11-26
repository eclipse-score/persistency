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

class DefaultValuesScenario final : public Scenario {
  public:
    ~DefaultValuesScenario() final = default;
    std::string name() const final;
    void run(const std::string& input) const final;
};

class RemoveKeyScenario final : public Scenario {
  public:
    ~RemoveKeyScenario() final = default;
    std::string name() const final;
    void run(const std::string& input) const final;
};

class ResetAllKeysScenario final : public Scenario {
  public:
    ~ResetAllKeysScenario() final = default;
    std::string name() const final;
    void run(const std::string& input) const final;
};

class ResetSingleKeyScenario final : public Scenario {
  public:
    ~ResetSingleKeyScenario() final = default;
    std::string name() const final;
    void run(const std::string& input) const final;
};

class ChecksumScenario final : public Scenario {
  public:
    ~ChecksumScenario() final = default;
    std::string name() const final;
    void run(const std::string& input) const final;
};

namespace test_default_values {
// Helper to get all scenarios
std::vector<std::shared_ptr<const Scenario>> get_default_value_scenarios();
// Default values group
ScenarioGroup::Ptr create_default_values_group();
} // namespace test_default_values

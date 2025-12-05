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

namespace test_multiple_kvs
{
    class MultipleInstanceIds : public Scenario
    {
    public:
        ~MultipleInstanceIds() final = default;
        std::string name() const final;
        void run(const std::string &input) const final;
    };
    class SameInstanceIdSameValue : public Scenario
    {
    public:
        ~SameInstanceIdSameValue() final = default;
        std::string name() const final;
        void run(const std::string &input) const final;
    };
    class SameInstanceIdDifferentValue : public Scenario
    {
    public:
        ~SameInstanceIdDifferentValue() final = default;
        std::string name() const final;
        void run(const std::string &input) const final;
    };
    ScenarioGroup::Ptr create_multiple_kvs_group();
}

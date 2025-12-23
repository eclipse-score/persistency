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

namespace test_snapshots
{
    class SnapshotCount : public Scenario
    {
    public:
        ~SnapshotCount() final = default;
        std::string name() const final;
        void run(const std::string &input) const final;
    };

    class SnapshotMaxCount : public Scenario
    {
    public:
        ~SnapshotMaxCount() final = default;
        std::string name() const final;
        void run(const std::string &input) const final;
    };

    class SnapshotRestore : public Scenario
    {
    public:
        ~SnapshotRestore() final = default;
        std::string name() const final;
        void run(const std::string &input) const final;
    };

    class SnapshotPaths : public Scenario
    {
    public:
        ~SnapshotPaths() final = default;
        std::string name() const final;
        void run(const std::string &input) const final;
    };

    ScenarioGroup::Ptr create_snapshots_group();
} // namespace test_snapshots

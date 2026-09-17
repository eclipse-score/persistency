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

#include <iostream>
#include <string>
#include <vector>

#include <cli.hpp>
#include <scenario.hpp>
#include <test_context.hpp>

#include "cit/cit.hpp"
#include "test_basic.hpp"

int main(int argc, char** argv)
{
    try
    {
        std::vector<std::string> raw_arguments{argv, argv + argc};

        // Basic group.
        Scenario::Ptr basic_scenario{new BasicScenario{}};
        ScenarioGroup::Ptr basic_group{new ScenarioGroupImpl{"basic", {basic_scenario}, {}}};

        // CIT group.
        ScenarioGroup::Ptr cit_group{cit_scenario_group()};

        // Root group.
        ScenarioGroup::Ptr root_group{new ScenarioGroupImpl{"root", {}, {basic_group, cit_group}}};

        // Run.
        TestContext test_context{root_group};
        run_cli_app(raw_arguments, test_context);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        // Match Rust panic return code.
        return 101;
    }
}

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

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "cit/test_default_values.hpp"
#include "cli.hpp"
#include "helpers/kvs_parameters.hpp"
#include "test_basic.hpp"

int main(int argc, char **argv)
{
    using namespace test_default_values;
    try
    {
        std::vector<std::string> raw_arguments{argv, argv + argc};

        // Basic group.
        Scenario::Ptr basic_scenario{new BasicScenario{}};
        ScenarioGroup::Ptr basic_group{new ScenarioGroupImpl{"basic", {basic_scenario}, {}}};
        ScenarioGroup::Ptr cit_group{
            new ScenarioGroupImpl{"cit", {}, {create_default_values_group()}}};
        // Root group.
        ScenarioGroup::Ptr root_group{new ScenarioGroupImpl{"root", {}, {basic_group, cit_group}}};

        // Run.
        TestContext test_context{root_group};
        run_cli_app(raw_arguments, test_context);
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
}

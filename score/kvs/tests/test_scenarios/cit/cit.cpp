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

#include "cit/cit.h"

#include "cit/default_values.h"
#include "cit/multiple_kvs.h"
#include "cit/snapshots.h"
#include "cit/supported_datatypes.h"

ScenarioGroup::Ptr cit_scenario_group()
{
    return ScenarioGroup::Ptr{new ScenarioGroupImpl{
        "cit", {}, {default_values_group(), multiple_kvs_group(), snapshots_group(), supported_datatypes_group()}}};
}

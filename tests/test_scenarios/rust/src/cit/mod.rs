// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************
use crate::cit::default_values::default_values_group;
use crate::cit::multiple_kvs::multiple_kvs_group;
use crate::cit::persistency::persistency_group;
use crate::cit::snapshots::snapshots_group;
use crate::cit::supported_datatypes::supported_datatypes_group;
use test_scenarios_rust::scenario::{ScenarioGroup, ScenarioGroupImpl};

mod default_values;
mod multiple_kvs;
mod persistency;
mod snapshots;
mod supported_datatypes;

/// Create a group containing scenarios for CITs.
pub fn cit_scenario_group() -> Box<dyn ScenarioGroup> {
    Box::new(ScenarioGroupImpl::new(
        "cit",
        vec![],
        vec![
            default_values_group(),
            multiple_kvs_group(),
            persistency_group(),
            snapshots_group(),
            supported_datatypes_group(),
        ],
    ))
}

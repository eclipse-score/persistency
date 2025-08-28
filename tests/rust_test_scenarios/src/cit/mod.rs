use crate::cit::supported_datatypes::supported_datatypes_group;
use test_scenarios_rust::scenario::{ScenarioGroup, ScenarioGroupImpl};

mod supported_datatypes;

pub fn cit_scenario_group() -> Box<dyn ScenarioGroup> {
    Box::new(ScenarioGroupImpl::new(
        "cit",
        vec![],
        vec![supported_datatypes_group()],
    ))
}

use test_scenarios_rust::cli::run_cli_app;
use test_scenarios_rust::scenario::ScenarioGroupImpl;
use test_scenarios_rust::test_context::TestContext;
mod cit;
mod helpers;
mod test_basic;
use crate::cit::cit_scenario_group;
use crate::test_basic::BasicScenario;

fn main() {
    let raw_arguments: Vec<String> = std::env::args().collect();

    // Basic group.
    let basic_scenario = Box::new(BasicScenario);
    let basic_group = Box::new(ScenarioGroupImpl::new(
        "basic",
        vec![basic_scenario],
        vec![],
    ));

    // CIT group.
    let cit_group = cit_scenario_group();

    // Root group.
    let root_group = Box::new(ScenarioGroupImpl::new(
        "root",
        vec![],
        vec![basic_group, cit_group],
    ));

    // Run.
    let test_context = TestContext::new(root_group);
    run_cli_app(&raw_arguments, &test_context);
}

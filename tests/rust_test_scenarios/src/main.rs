use test_scenarios_rust::cli::run_cli_app;
use test_scenarios_rust::scenario::ScenarioGroupImpl;
use test_scenarios_rust::test_context::TestContext;
mod cit;
mod helpers;
mod test_basic;
use crate::cit::cit_scenario_group;
use crate::test_basic::BasicScenario;
use std::time::{SystemTime, UNIX_EPOCH};
use tracing::Level;
use tracing_subscriber::fmt::time::FormatTime;
use tracing_subscriber::FmtSubscriber;

struct NumericUnixTime;

impl FormatTime for NumericUnixTime {
    fn format_time(
        &self,
        w: &mut tracing_subscriber::fmt::format::Writer<'_>,
    ) -> core::fmt::Result {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default();
        write!(w, "{}", now.as_secs())
    }
}

/// `tracing` is used for test outputs.
fn init_tracing_subscriber() {
    let subscriber = FmtSubscriber::builder()
        .with_max_level(Level::TRACE)
        .with_thread_ids(true)
        .with_timer(NumericUnixTime)
        .json()
        .finish();

    tracing::subscriber::set_global_default(subscriber)
        .expect("Setting default subscriber failed!");
}

/// Logging is used for regular logs.
fn init_logging() {
    #[cfg(feature = "stdout_logger")]
    stdout_logger::StdoutLoggerBuilder::new().set_as_default_logger();

    #[cfg(feature = "mw_logger")]
    mw_logger::MwLoggerBuilder::new().set_as_default_logger();
}

fn main() -> Result<(), String> {
    let raw_arguments: Vec<String> = std::env::args().collect();
    init_logging();

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
    init_tracing_subscriber();
    let test_context = TestContext::new(root_group);
    run_cli_app(&raw_arguments, &test_context)
}

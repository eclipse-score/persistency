use crate::helpers::kvs_instance::kvs_instance;
use crate::helpers::kvs_parameters::KvsParameters;
use rust_kvs::prelude::*;
use test_scenarios_rust::scenario::Scenario;
use tracing::info;

fn _error_code_to_string(e: ErrorCode) -> String {
    format!("ErrorCode::{e:?}")
}

pub struct BasicScenario;

/// Checks (almost) empty program with only shutdown
impl Scenario for BasicScenario {
    fn name(&self) -> &'static str {
        "basic"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        // Print and parse parameters.
        eprintln!("{input}");
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");

        // Create KVS.
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");

        // Simple set/get.
        let key = "example_key";
        let value = "example_value".to_string();
        kvs.set_value(key, value).expect("Failed to set value");
        let value_read = kvs
            .get_value_as::<String>(key)
            .expect("Failed to read value");

        // Trace.
        info!(example_key = value_read);

        Ok(())
    }
}

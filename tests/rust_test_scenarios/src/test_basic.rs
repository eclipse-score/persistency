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

    fn run(&self, input: Option<String>) -> Result<(), String> {
        // Print and parse parameters.
        eprintln!("{}", input.clone().unwrap());

        let params = KvsParameters::from_json(input.as_ref().unwrap()).unwrap();

        // Set builder parameters.
        let mut builder = KvsBuilder::new(params.instance_id);
        if let Some(flag) = params.need_defaults {
            builder = builder.need_defaults(match flag {
                OpenNeedDefaults::Optional => false,
                OpenNeedDefaults::Required => true,
            });
        }
        if let Some(flag) = params.need_kvs {
            builder = builder.need_kvs(match flag {
                OpenNeedKvs::Optional => false,
                OpenNeedKvs::Required => true,
            });
        }
        if let Some(dir) = params.dir {
            builder = builder.dir(dir.to_string_lossy().to_string());
        }

        // Create KVS.
        let kvs: Kvs = builder.build().unwrap();
        if let Some(flag) = params.flush_on_exit {
            kvs.set_flush_on_exit(flag);
        }

        // Simple set/get.
        let key = "example_key";
        let value = "example_value".to_string();
        kvs.set_value(key, value).unwrap();
        let value_read = kvs.get_value_as::<String>(key).unwrap();

        // Trace.
        info!(example_key = value_read);

        Ok(())
    }
}

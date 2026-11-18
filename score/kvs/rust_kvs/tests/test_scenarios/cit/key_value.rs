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
use crate::helpers::kvs_instance::kvs_instance;
use crate::helpers::kvs_parameters::KvsParameters;
use rust_kvs::prelude::*;
use serde_json::Value;
use test_scenarios_rust::scenario::{Scenario, ScenarioGroup, ScenarioGroupImpl};
use tracing::info;

/// Run one operation of the input on the KVS: `["set", key, I32 value]`, `["remove", key]` or `["reset"]`.
fn apply(kvs: &Kvs, operation: &[Value]) -> Result<(), ErrorCode> {
    match operation[0].as_str().expect("Operation name must be a string") {
        "set" => {
            let key = operation[1].as_str().expect("Key must be a string");
            let value = operation[2].as_i64().expect("Value must be an integer");
            kvs.set_value(key, i32::try_from(value).expect("Value must be an I32"))
        },
        "remove" => kvs.remove_key(operation[1].as_str().expect("Key must be a string")),
        "reset" => kvs.reset(),
        name => panic!("Unknown operation: {name}"),
    }
}

/// Open the KVS and run the operations of the input in memory, without a flush.
/// Then log every written key under "listed_key", and for each key of the input the results of
/// `key_exists`, `is_value_default` and `get_value`.
struct QueryAfterOperations;

impl Scenario for QueryAfterOperations {
    fn name(&self) -> &str {
        "query_after_operations"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");
        let input: Value = serde_json::from_str(input).expect("Failed to parse input");
        let operations = input["operations"].as_array().expect("Operations must be an array");
        let keys = input["keys"].as_array().expect("Keys must be an array");

        let kvs = kvs_instance(params).expect("Failed to create KVS instance");

        for operation in operations {
            apply(&kvs, operation.as_array().expect("Operation must be an array"))
                .unwrap_or_else(|e| panic!("Operation failed: {e:?}"));
        }

        for listed_key in kvs.get_all_keys().expect("Failed to get keys") {
            info!(listed_key = listed_key.as_str());
        }

        for key in keys {
            let key = key.as_str().expect("Key must be a string");
            let value = match kvs.get_value(key) {
                Ok(KvsValue::I32(number)) => format!("Ok({number})"),
                Ok(other) => panic!("Value is not an I32: {other:?}"),
                Err(e) => format!("Err({e:?})"),
            };
            info!(
                key,
                key_exists = format!("{:?}", kvs.key_exists(key)),
                value_is_default = format!("{:?}", kvs.is_value_default(key)),
                value
            );
        }

        Ok(())
    }
}

pub fn key_value_group() -> Box<dyn ScenarioGroup> {
    Box::new(ScenarioGroupImpl::new(
        "key_value",
        vec![Box::new(QueryAfterOperations)],
        vec![],
    ))
}

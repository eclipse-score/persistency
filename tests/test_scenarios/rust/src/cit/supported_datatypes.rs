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
use std::collections::HashMap;
use test_scenarios_rust::scenario::{Scenario, ScenarioGroup, ScenarioGroupImpl};
use tinyjson::JsonValue;
use tracing::info;

struct SupportedDatatypesKeys;

impl Scenario for SupportedDatatypesKeys {
    fn name(&self) -> &str {
        "keys"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");

        // Set key-value pairs. Unit type is used for value - only key is used later on.
        let keys_to_check = vec![
            String::from("example"),
            String::from("emoji ✅❗😀"),
            String::from("greek ημα"),
        ];
        for key in keys_to_check {
            kvs.set_value(key, ()).expect("Failed to set value");
        }

        // Get and print all keys.
        let keys_in_kvs = kvs.get_all_keys().expect("Failed to read all keys");
        for key in keys_in_kvs {
            info!(key);
        }

        Ok(())
    }
}

struct SupportedDatatypesValues {
    value: KvsValue,
}

impl Scenario for SupportedDatatypesValues {
    fn name(&self) -> &str {
        // Set scenario name based on provided value.
        match self.value {
            KvsValue::I32(_) => "i32",
            KvsValue::U32(_) => "u32",
            KvsValue::I64(_) => "i64",
            KvsValue::U64(_) => "u64",
            KvsValue::F64(_) => "f64",
            KvsValue::Boolean(_) => "bool",
            KvsValue::String(_) => "str",
            KvsValue::Null => "null",
            KvsValue::Array(_) => "arr",
            KvsValue::Object(_) => "obj",
        }
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");

        kvs.set_value(self.name(), self.value.clone())
            .expect("Failed to set value");

        let kvs_value = kvs.get_value(self.name()).expect("Failed to read value");
        let json_value = JsonValue::from(kvs_value);
        let json_str = json_value.stringify().expect("Failed to stringify JSON");

        info!(key = self.name(), value = json_str);

        Ok(())
    }
}

fn supported_datatypes_i32() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::I32(-321),
    })
}

fn supported_datatypes_u32() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::U32(1234),
    })
}

fn supported_datatypes_i64() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::I64(-123456789),
    })
}

fn supported_datatypes_u64() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::U64(123456789),
    })
}

fn supported_datatypes_f64() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::F64(-5432.1),
    })
}

fn supported_datatypes_bool() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::Boolean(true),
    })
}

fn supported_datatypes_string() -> Box<dyn Scenario> {
    Box::new(SupportedDatatypesValues {
        value: KvsValue::String("example".to_string()),
    })
}

fn supported_datatypes_array() -> Box<dyn Scenario> {
    let hashmap = HashMap::from([("sub-number".to_string(), KvsValue::from(789.0))]);
    let array = vec![
        KvsValue::from(321.5),
        KvsValue::from(false),
        KvsValue::from("hello".to_string()),
        KvsValue::from(()),
        KvsValue::from(vec![]),
        KvsValue::from(hashmap),
    ];
    Box::new(SupportedDatatypesValues {
        value: KvsValue::Array(array),
    })
}

fn supported_datatypes_object() -> Box<dyn Scenario> {
    let hashmap = HashMap::from([("sub-number".to_string(), KvsValue::from(789.0))]);
    Box::new(SupportedDatatypesValues {
        value: KvsValue::Object(hashmap),
    })
}

fn value_types_group() -> Box<dyn ScenarioGroup> {
    let group = ScenarioGroupImpl::new(
        "values",
        vec![
            supported_datatypes_i32(),
            supported_datatypes_u32(),
            supported_datatypes_i64(),
            supported_datatypes_u64(),
            supported_datatypes_f64(),
            supported_datatypes_bool(),
            supported_datatypes_string(),
            supported_datatypes_array(),
            supported_datatypes_object(),
        ],
        vec![],
    );
    Box::new(group)
}

struct ValueLength;

impl Scenario for ValueLength {
    fn name(&self) -> &str {
        "ValueLength"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let byte_size: usize = serde_json::from_value(v["byte_size"].clone())
            .expect("Failed to parse \"byte_size\" field");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");

        let kvs = kvs_instance(params).expect("Failed to create KVS instance");

        // Create a string of specified byte size
        let test_value = "x".repeat(byte_size);
        let actual_size = test_value.len();
        info!(byte_size, actual_size, "Testing value length");

        // Attempt to store the value
        let store_result = kvs.set_value("test_key", test_value.clone()).is_ok();
        info!(store_result, "Store operation result");

        if store_result {
            // If store succeeded, try to retrieve and verify
            match kvs.get_value("test_key") {
                Ok(retrieved_value) => {
                    if let KvsValue::String(retrieved_str) = retrieved_value {
                        let value_size = retrieved_str.len();
                        info!(retrieve_success = true, value_size, "Retrieved value");
                    } else {
                        info!(retrieve_success = false, "Retrieved value is not a string");
                    }
                }
                Err(e) => {
                    info!(retrieve_success = false, error = ?e, "Failed to retrieve value");
                }
            }
        } else {
            info!(retrieve_success = false, "Store failed, skipping retrieval");
        }

        Ok(())
    }
}

pub fn supported_datatypes_group() -> Box<dyn ScenarioGroup> {
    Box::new(ScenarioGroupImpl::new(
        "supported_datatypes",
        vec![Box::new(SupportedDatatypesKeys), Box::new(ValueLength)],
        vec![value_types_group()],
    ))
}

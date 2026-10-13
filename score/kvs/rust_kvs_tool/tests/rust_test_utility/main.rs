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

use rust_kvs::prelude::*;
use std::collections::HashMap;
use std::path::PathBuf;

/// Execute scenario for `store` test.
fn store_scenario(working_dir: PathBuf) {
    // Initialize KVS.
    let instance_id = InstanceId(0);
    let backend = Box::new(JsonBackendBuilder::new().working_dir(working_dir).build());
    let kvs = KvsBuilder::new(instance_id)
        .defaults(KvsDefaults::Ignored)
        .kvs_load(KvsLoad::Required)
        .backend(backend)
        .build()
        .unwrap();

    // Check structure is as expected.
    assert_eq!(kvs.get_value_as::<f64>("number").unwrap(), 123.0f64);
    assert_eq!(kvs.get_value_as::<bool>("bool").unwrap(), true);
    assert_eq!(kvs.get_value_as::<String>("string").unwrap(), "First");
    assert_eq!(kvs.get_value_as::<()>("null").unwrap(), ());
    assert_eq!(
        kvs.get_value_as::<Vec<KvsValue>>("array").unwrap(),
        vec![KvsValue::from(456.0), false.into(), "Second".to_string().into()]
    );
    assert_eq!(
        kvs.get_value_as::<HashMap<String, KvsValue>>("object").unwrap(),
        HashMap::from([
            (String::from("sub-number"), KvsValue::from(789.0)),
            ("sub-bool".into(), true.into()),
            ("sub-string".into(), "Third".to_string().into()),
            ("sub-null".into(), ().into()),
            (
                "sub-array".into(),
                KvsValue::from(vec![KvsValue::from(1246.0), false.into(), "Fourth".to_string().into()]),
            ),
        ])
    );

    println!("PASSED");
}

/// Execute scenario for `load` test.
fn load_scenario(working_dir: PathBuf) {
    // Initialize KVS.
    let instance_id = InstanceId(0);
    let backend = Box::new(JsonBackendBuilder::new().working_dir(working_dir).build());
    let kvs = KvsBuilder::new(instance_id)
        .defaults(KvsDefaults::Ignored)
        .kvs_load(KvsLoad::Ignored)
        .backend(backend)
        .build()
        .unwrap();

    // Set structure.
    kvs.set_value("i32", -123i32).unwrap();
    kvs.set_value("u32", 123u32).unwrap();
    kvs.set_value("i64", -12345i64).unwrap();
    kvs.set_value("u64", 12345u64).unwrap();
    kvs.set_value("f64", 12345.6f64).unwrap();
    kvs.set_value("bool", true).unwrap();
    kvs.set_value("string", "string-value").unwrap();
    kvs.set_value("null", ()).unwrap();
    kvs.set_value("array", vec![555i64.into(), "string-in-array".into()])
        .unwrap();
    kvs.set_value("object", HashMap::from([("f64-in-object".into(), 123.456f64.into())]))
        .unwrap();

    // Flush.
    kvs.flush().unwrap();
}

fn main() {
    // Get and process arguments.
    let args: Vec<_> = std::env::args_os().map(|s| s.to_str().unwrap().to_string()).collect();
    if args.len() < 3 {
        panic!("invalid number of arguments");
    }
    let test_scenario_name = args.get(1).unwrap();
    let working_dir = PathBuf::from(args.get(2).unwrap());

    // Run selected scenario.
    match test_scenario_name.as_str() {
        "store" => store_scenario(working_dir),
        "load" => load_scenario(working_dir),
        _ => panic!("invalid test scenario name"),
    }
}

// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

//! # Verify KVS Default Value Functionality

use rust_kvs::prelude::*;
use std::collections::HashMap;
use std::env::set_current_dir;
use tempfile::tempdir;
use tinyjson::{JsonGenerator, JsonValue};

/// Test default values
///   * Default file must exist
///   * Default value must be returned when key isn't set
///   * Key must report that default is used
///   * Key must be returned when it was written and report it
///   * Change in default must be returned when key isn't set
///   * Change in default must be ignored when key was once set
#[test]
fn kvs_without_defaults() -> Result<(), ErrorCode> {
    let dir = tempdir()?;
    set_current_dir(dir.path())?;

    // create defaults file in t-tagged format
    let defaults: HashMap<String, JsonValue> = HashMap::from([
        (
            "number1".to_string(),
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("f64".to_string())),
                ("v".to_string(), JsonValue::Number(123.0)),
            ])),
        ),
        (
            "bool1".to_string(),
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("bool".to_string())),
                ("v".to_string(), JsonValue::Boolean(true)),
            ])),
        ),
        (
            "string1".to_string(),
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("str".to_string())),
                ("v".to_string(), JsonValue::String("Hello".to_string())),
            ])),
        ),
    ]);

    let json_value = JsonValue::Object(defaults);
    let mut buf = Vec::new();
    let mut gen = JsonGenerator::new(&mut buf).indent("  ");
    gen.generate(&json_value)?;

    let data = String::from_utf8(buf)?;
    let filepath = &dir.path().join(format!("kvs_{}_default.json", 0));
    std::fs::write(filepath, &data)?;

    // create KVS
    let kvs = KvsBuilder::<Kvs>::new(InstanceId::new(0))
        .need_defaults(true)
        .need_kvs(false)
        .build()?;

    kvs.set_value("number2", 345.0)?;
    kvs.set_value("bool2", false)?;
    kvs.set_value("string2", "Ola".to_string())?;

    assert_eq!(kvs.get_value_as::<f64>("number1")?, 123.0);
    assert_eq!(kvs.get_value_as::<f64>("number2")?, 345.0);

    assert!(kvs.get_value_as::<bool>("bool1")?);
    assert!(!kvs.get_value_as::<bool>("bool2")?);

    assert_eq!(kvs.get_value_as::<String>("string1")?, "Hello".to_string());
    assert_eq!(kvs.get_value_as::<String>("string2")?, "Ola".to_string());

    assert!(kvs.is_value_default("number1")?);
    assert!(!kvs.is_value_default("number2")?);

    assert!(kvs.is_value_default("bool1")?);
    assert!(!kvs.is_value_default("bool2")?);

    assert!(kvs.is_value_default("string1")?);
    assert!(!kvs.is_value_default("string2")?);

    // write same-as-default-value into `bool1`
    kvs.set_value("bool1", true)?;

    // write not-same-as-default into `string1`
    kvs.set_value("string1", "Bonjour".to_string())?;

    // drop the current instance with flush-on-exit enabled and reopen storage
    drop(kvs);

    let kvs = KvsBuilder::<Kvs>::new(InstanceId::new(0))
        .need_defaults(false)
        .need_kvs(true)
        .build()?;

    assert!(kvs.get_value_as::<bool>("bool1")?);
    assert!(!kvs.is_value_default("bool1")?);

    assert_eq!(
        kvs.get_value_as::<String>("string1")?,
        "Bonjour".to_string()
    );
    assert!(!kvs.is_value_default("string1")?);

    // drop the current instance with flush-on-exit enabled and reopen storage
    drop(kvs);

    // create defaults file in t-tagged format
    let defaults: HashMap<String, JsonValue> = HashMap::from([
        (
            "number1".to_string(),
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("f64".to_string())),
                ("v".to_string(), JsonValue::Number(987.0)),
            ])),
        ),
        (
            "bool1".to_string(),
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("bool".to_string())),
                ("v".to_string(), JsonValue::Boolean(false)),
            ])),
        ),
        (
            "string1".to_string(),
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("str".to_string())),
                ("v".to_string(), JsonValue::String("Hello".to_string())),
            ])),
        ),
    ]);

    let json_value = JsonValue::from(defaults);
    let mut buf = Vec::new();
    let mut gen = JsonGenerator::new(&mut buf).indent("  ");
    gen.generate(&json_value)?;

    let data = String::from_utf8(buf)?;
    std::fs::write(filepath, &data)?;

    let kvs = KvsBuilder::<Kvs>::new(InstanceId::new(0))
        .need_defaults(false)
        .need_kvs(true)
        .build()?;

    assert_eq!(kvs.get_value_as::<f64>("number1")?, 987.0);
    assert!(kvs.is_value_default("number1")?);

    assert!(kvs.get_value_as::<bool>("bool1")?);
    assert!(!kvs.is_value_default("bool1")?);

    Ok(())
}

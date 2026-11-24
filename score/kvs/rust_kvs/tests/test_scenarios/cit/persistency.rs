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
use std::fs;
use std::hash::{DefaultHasher, Hash, Hasher};
use std::path::Path;
use test_scenarios_rust::scenario::{Scenario, ScenarioGroup, ScenarioGroupImpl};
use tracing::info;

/// Every file in the directory as "name=fingerprint", sorted and joined with ','.
///
/// The fingerprint is a hash of the file content, comparable within one run only.
fn storage_fingerprints(dir: &Path) -> String {
    let mut entries: Vec<String> = fs::read_dir(dir)
        .expect("Failed to read working directory")
        .map(|entry| {
            let entry = entry.expect("Failed to read directory entry");
            let mut hasher = DefaultHasher::new();
            fs::read(entry.path()).expect("Failed to read file").hash(&mut hasher);
            format!("{}={}", entry.file_name().to_string_lossy(), hasher.finish())
        })
        .collect();
    entries.sort();
    entries.join(",")
}

/// Log every written key of the KVS with its I32 value.
fn log_stored_values(kvs: &Kvs) {
    for key in kvs.get_all_keys().expect("Failed to get keys") {
        let value: i32 = kvs.get_value_as(&key).expect("Failed to read value as I32");
        info!(key = key.as_str(), value);
    }
}

/// Change the KVS, flush it twice, change it again without flush, then drop the KVS object.
///
/// The second flush overwrites and removes keys already in storage.
/// The changes after it end with a snapshot restore, which replaces the in-memory data.
/// Logs the files in the working directory with their fingerprints after each step.
struct ExplicitFlush;

impl Scenario for ExplicitFlush {
    fn name(&self) -> &str {
        "explicit_flush"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");
        let dir = params.dir.clone().expect("Working directory must be set");

        // Panics if the step failed, then logs the files in the working directory with their fingerprints.
        let step = |name: &str, result: Result<(), ErrorCode>| {
            result.unwrap_or_else(|e| panic!("Failed to {name}: {e:?}"));
            info!(step = name, storage = storage_fingerprints(&dir));
        };

        let kvs = kvs_instance(params).expect("Failed to create KVS instance");
        step("open", Ok(()));

        // Changes before the first flush, on empty storage: set.
        step("set", kvs.set_value("kept", 1));
        step("set", kvs.set_value("overwritten", 1));
        step("set", kvs.set_value("removed", 3));
        step("flush", kvs.flush());

        // Changes to stored keys, then the second flush: overwrite, remove.
        step("set", kvs.set_value("overwritten", 2));
        step("remove", kvs.remove_key("removed"));
        step("flush", kvs.flush());

        // Changes after the last flush, never flushed: overwrite, remove, reset, set, snapshot restore.
        // Snapshot 1 holds the state of the first flush.
        step("set", kvs.set_value("overwritten", 20));
        step("remove", kvs.remove_key("kept"));
        step("reset", kvs.reset());
        step("set", kvs.set_value("added", 4));
        step("restore", kvs.snapshot_restore(SnapshotId(1)));

        // The KVS object is dropped: a file written now was written by the drop.
        drop(kvs);
        step("drop", Ok(()));

        Ok(())
    }
}

/// Open the KVS and log every written key with its I32 value.
struct StoredValues;

impl Scenario for StoredValues {
    fn name(&self) -> &str {
        "stored_values"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");
        log_stored_values(&kvs);
        Ok(())
    }
}

/// Open the KVS, restore snapshot 1 and log the error if the restore fails.
/// Then log every written key with its I32 value.
struct ValuesAfterRestore;

impl Scenario for ValuesAfterRestore {
    fn name(&self) -> &str {
        "values_after_restore"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let params = KvsParameters::from_json(input).expect("Failed to parse parameters");
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");
        if let Err(e) = kvs.snapshot_restore(SnapshotId(1)) {
            info!(load_error = format!("{e:?}"));
        }
        log_stored_values(&kvs);
        Ok(())
    }
}

/// Set one key per value type, then flush as often as the input field "flush_count" says.
///
/// Before each flush, set the key "generation" to the number of the flush, 1 for the first.
/// The string value and the key `quote" backslash\` hold characters that JSON must escape.
struct FlushValueTypes;

impl Scenario for FlushValueTypes {
    fn name(&self) -> &str {
        "flush_value_types"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let flush_count: i32 =
            serde_json::from_value(v["flush_count"].clone()).expect("Failed to parse \"flush_count\" field");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");

        let values = [
            ("i32", KvsValue::I32(-321)),
            ("u32", KvsValue::U32(1234)),
            ("i64", KvsValue::I64(-123456789012)),
            ("u64", KvsValue::U64(123456789012)),
            ("f64", KvsValue::F64(-5432.1)),
            ("bool", KvsValue::Boolean(true)),
            ("null", KvsValue::Null),
            (
                "str",
                KvsValue::String(
                    "quote \" backslash \\ slash / newline \n tab \t control \u{1} umlaut \u{e4}".to_string(),
                ),
            ),
            (
                "arr",
                KvsValue::Array(vec![
                    KvsValue::I32(1),
                    KvsValue::String(String::new()),
                    KvsValue::Boolean(false),
                    KvsValue::Null,
                    KvsValue::Array(vec![]),
                    KvsValue::Object(KvsMap::new()),
                ]),
            ),
            (
                "obj",
                KvsValue::Object(KvsMap::from([
                    ("nested".to_string(), KvsValue::F64(1.5)),
                    (
                        "deeper".to_string(),
                        KvsValue::Object(KvsMap::from([(
                            "text".to_string(),
                            KvsValue::String("value".to_string()),
                        )])),
                    ),
                ])),
            ),
            ("quote\" backslash\\", KvsValue::I32(7)),
        ];
        for (key, value) in values {
            kvs.set_value(key, value)
                .unwrap_or_else(|e| panic!("Failed to set {key}: {e:?}"));
        }

        for generation in 1..=flush_count {
            kvs.set_value("generation", generation)
                .expect("Failed to set generation");
            kvs.flush().expect("Failed to flush");
        }

        Ok(())
    }
}

/// Recover the data from a snapshot, as an application does when the open finds the storage corrupted.
///
/// Open the KVS with load mode Required. If the open succeeds, log "required_open" = "succeeded" and end: there is
/// nothing to recover. If the open fails, log the error under "open_error".
/// Then open it with load mode Ignored, restore the snapshot of the input field "snapshot_id" and log the error if the
/// restore fails, log every written key with its I32 value, flush and log the error under "flush_error" if the flush
/// fails.
struct RecoverFromSnapshot;

impl Scenario for RecoverFromSnapshot {
    fn name(&self) -> &str {
        "recover_from_snapshot"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let snapshot_id: usize =
            serde_json::from_value(v["snapshot_id"].clone()).expect("Failed to parse \"snapshot_id\" field");
        let mut params = KvsParameters::from_value(&v).expect("Failed to parse parameters");

        params.kvs_load = Some(KvsLoad::Required);
        match kvs_instance(params.clone()) {
            // Nothing to recover. An open with load mode Ignored would also fail: the instance pool keeps this
            // instance with load mode Required.
            Ok(_) => {
                info!(required_open = "succeeded");
                return Ok(());
            },
            // The same text as the exception of the C++ scenario.
            Err(e) => info!(open_error = format!("Failed to create KVS instance: {e:?}")),
        }

        params.kvs_load = Some(KvsLoad::Ignored);
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");
        if let Err(e) = kvs.snapshot_restore(SnapshotId(snapshot_id)) {
            info!(load_error = format!("{e:?}"));
        }
        log_stored_values(&kvs);

        if let Err(e) = kvs.flush() {
            info!(flush_error = format!("{e:?}"));
        }

        Ok(())
    }
}

pub fn persistency_group() -> Box<dyn ScenarioGroup> {
    Box::new(ScenarioGroupImpl::new(
        "persistency",
        vec![
            Box::new(ExplicitFlush),
            Box::new(StoredValues),
            // There is no counterpart of the C++ scenario `values_after_discard`: the Rust KVS has no API to discard
            // pending changes.
            Box::new(ValuesAfterRestore),
            Box::new(FlushValueTypes),
            Box::new(RecoverFromSnapshot),
        ],
        vec![],
    ))
}

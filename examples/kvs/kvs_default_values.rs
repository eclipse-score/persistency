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
//! Showcase: KVS Default Values
//!
//! Demonstrates factory defaults as a safe fallback layer beneath runtime writes.
//! Shows how to open a KVS instance with a pre-configured defaults file, how
//! explicit writes override defaults, and how `reset_key` restores the factory value.

use rust_kvs::prelude::*;
use std::path::PathBuf;
use tempfile::tempdir;
use tinyjson::JsonValue;
use tracing::info;

fn init_tracing() {
    let subscriber = tracing_subscriber::fmt().with_max_level(tracing::Level::INFO).finish();
    tracing::subscriber::set_global_default(subscriber).expect("Setting default subscriber failed!");
}

fn write_defaults_file(dir: PathBuf, instance_id: InstanceId) -> Result<(), ErrorCode> {
    let json_path = dir.join(format!("kvs_{instance_id}_default.json"));
    let hash_path = dir.join(format!("kvs_{instance_id}_default.hash"));

    let defaults = KvsValue::from(KvsMap::from([
        ("max_speed".to_string(), KvsValue::I32(120i32)),
        ("drive_mode".to_string(), KvsValue::String("economy".to_string())),
    ]));

    let json_str = JsonValue::from(defaults).stringify()?;
    std::fs::write(&json_path, &json_str)?;

    let hash = adler32::RollingAdler32::from_buffer(json_str.as_bytes()).hash();
    std::fs::write(hash_path, hash.to_be_bytes())?;

    Ok(())
}

fn main() -> Result<(), ErrorCode> {
    #[cfg(feature = "score_log_bridge")]
    score_log_bridge::ScoreLogBridgeBuilder::new().set_as_default_logger();
    init_tracing();

    let dir = tempdir()?;
    let instance_id = InstanceId(0);

    info!("=== KVS Default Values ===");

    // Write a defaults file containing factory-safe values.
    write_defaults_file(dir.path().to_path_buf(), instance_id)?;
    info!("[1] Defaults file written: max_speed=120, drive_mode=\"economy\"");

    let kvs = KvsBuilder::new(instance_id)
        .backend(Box::new(
            JsonBackendBuilder::new().working_dir(dir.path().to_path_buf()).build(),
        ))
        .defaults(KvsDefaults::Required)
        .build()?;

    // Before any write: both keys return their default values.
    let max_speed_val = kvs.get_value("max_speed")?;
    let max_speed_is_default = kvs.is_value_default("max_speed")?;
    let drive_mode_val = kvs.get_value("drive_mode")?;
    let drive_mode_is_default = kvs.is_value_default("drive_mode")?;
    info!("[2] Values before any explicit write (defaults active)");
    info!(max_speed = ?max_speed_val, is_default = max_speed_is_default);
    info!(drive_mode = ?drive_mode_val, is_default = drive_mode_is_default);

    assert!(max_speed_is_default, "max_speed should be at default");
    assert!(drive_mode_is_default, "drive_mode should be at default");

    // Override max_speed at runtime.
    kvs.set_value("max_speed", 200i32)?;
    let current = kvs.get_value("max_speed")?;
    let default = kvs.get_default_value("max_speed")?;
    let is_default = kvs.is_value_default("max_speed")?;
    info!("[3] After override: max_speed");
    info!(current_value = ?current, default_value = ?default, is_default);

    assert!(!is_default, "max_speed should not be at default after override");

    // Reset max_speed back to factory default.
    kvs.reset_key("max_speed")?;
    let after_reset = kvs.get_value("max_speed")?;
    let is_default_after_reset = kvs.is_value_default("max_speed")?;
    info!("[4] After reset_key: max_speed");
    info!(value = ?after_reset, is_default = is_default_after_reset);

    assert!(is_default_after_reset, "max_speed should be at default after reset");

    info!("[5] Default values, override, and reset — all verified");

    Ok(())
}

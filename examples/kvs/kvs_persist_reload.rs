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
//! Showcase: KVS Persist and Reload
//!
//! Demonstrates writing values of different types into a KVS instance,
//! flushing them to disk, and reading them back from a newly opened instance —
//! verifying that data survives across instance boundaries (non-volatile storage).

use rust_kvs::prelude::*;
use tempfile::tempdir;
use tracing::info;

fn init_tracing() {
    let subscriber = tracing_subscriber::fmt().with_max_level(tracing::Level::INFO).finish();
    tracing::subscriber::set_global_default(subscriber).expect("Setting default subscriber failed!");
}

fn main() -> Result<(), ErrorCode> {
    #[cfg(feature = "score_log_bridge")]
    score_log_bridge::ScoreLogBridgeBuilder::new().set_as_default_logger();
    init_tracing();

    let dir = tempdir()?;
    let dir_path = dir.path().to_path_buf();
    let instance_id = InstanceId(0);

    info!("=== KVS Persist and Reload ===");

    // Phase 1: Write values and flush to disk.
    {
        info!("[1] Opening KVS instance and writing values");

        let kvs = KvsBuilder::new(instance_id)
            .backend(Box::new(
                JsonBackendBuilder::new().working_dir(dir_path.clone()).build(),
            ))
            .build()?;

        kvs.set_value("speed_limit", 130i32)?;
        kvs.set_value("unit", "km/h")?;
        kvs.set_value("eco_mode", false)?;

        info!(
            speed_limit = ?kvs.get_value("speed_limit")?,
            unit = ?kvs.get_value("unit")?,
            eco_mode = ?kvs.get_value("eco_mode")?
        );

        kvs.flush()?;
        info!("[2] Flushed to disk — instance closed");
    }
    // KVS instance dropped here; in-memory state is gone.

    // Phase 2: Reopen from disk and verify values are intact.
    info!("[3] Reopening KVS instance from same storage path");
    {
        let kvs = KvsBuilder::new(instance_id)
            .backend(Box::new(JsonBackendBuilder::new().working_dir(dir_path).build()))
            .build()?;

        let speed = kvs.get_value_as::<i32>("speed_limit")?;
        let unit = kvs.get_value_as::<String>("unit")?;
        let eco = kvs.get_value_as::<bool>("eco_mode")?;

        info!(speed_limit = speed, unit, eco_mode = eco);

        assert_eq!(speed, 130, "speed_limit did not survive reload");
        assert_eq!(unit, "km/h", "unit did not survive reload");
        assert!(!eco, "eco_mode did not survive reload");

        info!("[4] All values reloaded and verified — write, flush, reopen pattern demonstrated");
    }

    Ok(())
}

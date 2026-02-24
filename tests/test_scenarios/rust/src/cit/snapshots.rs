use crate::helpers::kvs_hash_paths;
use crate::helpers::kvs_instance::kvs_instance;
use crate::helpers::kvs_parameters::KvsParameters;
use rust_kvs::prelude::{KvsApi, SnapshotId};
use serde_json::Value;
use test_scenarios_rust::scenario::{Scenario, ScenarioGroup, ScenarioGroupImpl};
use tracing::info;

struct SnapshotCount;

impl Scenario for SnapshotCount {
    fn name(&self) -> &str {
        "count"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let count =
            serde_json::from_value(v["count"].clone()).expect("Failed to parse \"count\" field");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");

        // Create snapshots.
        for i in 0..count {
            let kvs = kvs_instance(params.clone()).expect("Failed to create KVS instance");
            kvs.set_value("counter", i).expect("Failed to set value");
            info!(snapshot_count = kvs.snapshot_count());

            // Flush KVS.
            kvs.flush().expect("Failed to flush");
        }

        {
            let kvs = kvs_instance(params).expect("Failed to create KVS instance");
            info!(snapshot_count = kvs.snapshot_count());
        }

        Ok(())
    }
}

struct SnapshotMaxCount;

impl Scenario for SnapshotMaxCount {
    fn name(&self) -> &str {
        "max_count"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");

        let kvs = kvs_instance(params.clone()).expect("Failed to create KVS instance");
        info!(max_count = kvs.snapshot_max_count());
        Ok(())
    }
}

struct SnapshotRestore;

impl Scenario for SnapshotRestore {
    fn name(&self) -> &str {
        "restore"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let count =
            serde_json::from_value(v["count"].clone()).expect("Failed to parse \"count\" field");
        let snapshot_id = serde_json::from_value(v["snapshot_id"].clone())
            .expect("Failed to parse \"snapshot_id\" field");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");

        // Create snapshots.
        for i in 0..count {
            let kvs = kvs_instance(params.clone()).expect("Failed to create KVS instance");
            kvs.set_value("counter", i).expect("Failed to set value");

            // Flush KVS.
            kvs.flush().expect("Failed to flush");
        }

        {
            let kvs = kvs_instance(params).expect("Failed to create KVS instance");

            let result = kvs.snapshot_restore(SnapshotId(snapshot_id));
            info!(result = format!("{result:?}"));
            if let Ok(()) = result {
                let value = kvs
                    .get_value_as::<i32>("counter")
                    .expect("Failed to read value");
                info!(value);
            }
        }

        Ok(())
    }
}

struct SnapshotPaths;

impl Scenario for SnapshotPaths {
    fn name(&self) -> &str {
        "paths"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let count =
            serde_json::from_value(v["count"].clone()).expect("Failed to parse \"count\" field");
        let snapshot_id = serde_json::from_value(v["snapshot_id"].clone())
            .expect("Failed to parse \"snapshot_id\" field");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");
        let instance_id = params.instance_id;
        let working_dir = params.dir.clone().expect("Working directory must be set");

        // Create snapshots.
        for i in 0..count {
            let kvs = kvs_instance(params.clone()).expect("Failed to create KVS instance");
            kvs.set_value("counter", i).expect("Failed to set value");

            // Flush KVS.
            kvs.flush().expect("Failed to flush");
        }

        {
            let (kvs_path, hash_path) =
                kvs_hash_paths(&working_dir, instance_id, SnapshotId(snapshot_id));
            info!(
                kvs_path = format!("{}", kvs_path.display()),
                hash_path = format!("{}", hash_path.display())
            );
        }

        Ok(())
    }
}

struct SnapshotIDAssignment;

impl Scenario for SnapshotIDAssignment {
    fn name(&self) -> &str {
        "id_assignment"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let count: i32 =
            serde_json::from_value(v["count"].clone()).expect("Failed to parse \"count\" field");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");

        // Create snapshots and track their IDs.
        for i in 0..count {
            let kvs = kvs_instance(params.clone()).expect("Failed to create KVS instance");
            kvs.set_value("counter", i).expect("Failed to set value");
            kvs.flush().expect("Failed to flush");
        }

        // Verify snapshot IDs exist
        let kvs = kvs_instance(params.clone()).expect("Failed to create KVS instance");
        let snapshot_count = kvs.snapshot_count();
        info!(snapshot_ids = format!("count={}", snapshot_count));

        Ok(())
    }
}

struct SnapshotDeletion;

impl Scenario for SnapshotDeletion {
    fn name(&self) -> &str {
        "deletion"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let count: i32 =
            serde_json::from_value(v["count"].clone()).expect("Failed to parse \"count\" field");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");
        let snapshot_max_count = kvs_instance(params.clone())
            .expect("Failed to create KVS instance")
            .snapshot_max_count();

        // Create more snapshots than the maximum to trigger deletion.
        for i in 0..count {
            let kvs = kvs_instance(params.clone()).expect("Failed to create KVS instance");
            kvs.set_value("counter", i).expect("Failed to set value");
            kvs.flush().expect("Failed to flush");
        }

        // Verify that only max_count snapshots exist
        let kvs = kvs_instance(params).expect("Failed to create KVS instance");
        let final_snapshot_count = kvs.snapshot_count();

        info!(
            oldest_deleted =
                final_snapshot_count == snapshot_max_count && count > snapshot_max_count as i32
        );

        Ok(())
    }
}

struct SnapshotNoVersioning;

impl Scenario for SnapshotNoVersioning {
    fn name(&self) -> &str {
        "no_versioning"
    }

    fn run(&self, input: &str) -> Result<(), String> {
        let v: Value = serde_json::from_str(input).expect("Failed to parse input string");
        let params = KvsParameters::from_value(&v).expect("Failed to parse parameters");

        // Create a KVS and flush it
        let kvs = kvs_instance(params.clone()).expect("Failed to create KVS instance");
        kvs.set_value("test_key", 42).expect("Failed to set value");
        kvs.flush().expect("Failed to flush");

        // Read the JSON file and verify no version field exists
        let instance_id = params.instance_id;
        let working_dir = params.dir.expect("Working directory must be set");
        let (kvs_path, _) = kvs_hash_paths(&working_dir, instance_id, SnapshotId(0));

        let file_content = std::fs::read_to_string(&kvs_path).expect("Failed to read KVS file");
        let kvs_data: Value =
            serde_json::from_str(&file_content).expect("Failed to parse KVS JSON");

        // Check that there's no 'version' field
        let no_version_field = !kvs_data
            .as_object()
            .is_some_and(|obj| obj.contains_key("version"));
        info!(no_version_field);

        Ok(())
    }
}

pub fn snapshots_group() -> Box<dyn ScenarioGroup> {
    Box::new(ScenarioGroupImpl::new(
        "snapshots",
        vec![
            Box::new(SnapshotCount),
            Box::new(SnapshotMaxCount),
            Box::new(SnapshotRestore),
            Box::new(SnapshotPaths),
            Box::new(SnapshotIDAssignment),
            Box::new(SnapshotDeletion),
            Box::new(SnapshotNoVersioning),
        ],
        vec![],
    ))
}

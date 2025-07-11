//! Persistency tests.
//!
//! Requirements verified:
//! - Persistency (feat_req__persistency__persistency)
//! The KVS system shall persist stored data and provide an API to explicitly trigger persistence.
//! - Store persistent data (feat_req__persistency__persist_data)
//! The KVS shall support storing and loading its data to and from persistent storage.

use rust_kvs::{ErrorCode, InstanceId, Kvs, KvsApi, KvsValue, OpenNeedDefaults, OpenNeedKvs};
use std::collections::HashMap;
use std::iter::zip;
use tempfile::tempdir;

fn compare_kvs_values(left: &KvsValue, right: &KvsValue) -> bool {
    match (left, right) {
        (KvsValue::Number(l), KvsValue::Number(r)) => l == r,
        (KvsValue::Boolean(l), KvsValue::Boolean(r)) => l == r,
        (KvsValue::String(l), KvsValue::String(r)) => l == r,
        (KvsValue::Null, KvsValue::Null) => true,
        (KvsValue::Array(l), KvsValue::Array(r)) => {
            // Check size.
            if l.len() != r.len() {
                return false;
            }

            // Iterate over elements.
            for (lv, rv) in zip(l, r) {
                if !compare_kvs_values(lv, rv) {
                    return false;
                }
            }

            true
        }
        (KvsValue::Object(l), KvsValue::Object(r)) => {
            // Check size.
            if l.len() != r.len() {
                return false;
            }

            // Check keys.
            if l.keys().ne(r.keys()) {
                return false;
            }

            // Iterate over elements.
            let keys = l.keys();
            for k in keys {
                if !compare_kvs_values(&l[k], &r[k]) {
                    return false;
                }
            }

            true
        }
        (_, _) => false,
    }
}

/// Flush on exit is enabled by default.
/// Data will be flushed on `kvs` being dropped.
#[test]
fn cit_persistency_flush_on_exit_enabled() -> Result<(), ErrorCode> {
    // Temp directory.
    let dir = tempdir()?;
    let dir_path = dir.path().to_string_lossy().to_string();

    // Values of each type.
    let mut kv_values: HashMap<String, KvsValue> = HashMap::new();
    kv_values.insert("number".to_string(), KvsValue::from(123.4));
    kv_values.insert("bool".to_string(), KvsValue::from(true));
    kv_values.insert("str".to_string(), KvsValue::from("abcd".to_string()));
    kv_values.insert("null".to_string(), KvsValue::from(()));
    let hashmap = HashMap::from([("sub-number".to_string(), KvsValue::from(789.0))]);
    kv_values.insert("obj".to_string(), KvsValue::from(hashmap.clone()));
    let array = vec![
        KvsValue::from(321.0),
        KvsValue::from(false),
        KvsValue::from("dbca".to_string()),
        KvsValue::from(()),
        KvsValue::from(vec![]),
        KvsValue::from(hashmap),
    ];
    kv_values.insert("array".to_string(), KvsValue::from(array));

    {
        // First KVS run.
        let kvs = Kvs::open(
            InstanceId::new(0),
            OpenNeedDefaults::Optional,
            OpenNeedKvs::Optional,
            Some(dir_path.clone()),
        )?;

        // Set values.
        for (key, value) in kv_values.iter() {
            kvs.set_value(key, value.clone())?;
        }
    }

    // Assertions.
    {
        // Second KVS run.
        // KVS file is expected to exist.
        let kvs = Kvs::open(
            InstanceId::new(0),
            OpenNeedDefaults::Optional,
            OpenNeedKvs::Required,
            Some(dir_path),
        )?;

        // Compare values.
        for (key, expected_value) in kv_values.iter() {
            let actual_value = kvs.get_value(&key).unwrap();
            assert!(compare_kvs_values(expected_value, &actual_value))
        }
    }

    Ok(())
}

#[test]
fn cit_persistency_flush_on_exit_disabled_drop_data() -> Result<(), ErrorCode> {
    // Temp directory.
    let dir = tempdir()?;
    let dir_path = dir.path().to_string_lossy().to_string();

    // Values of each type.
    let mut kv_values: HashMap<String, KvsValue> = HashMap::new();
    kv_values.insert("number".to_string(), KvsValue::from(123.4));
    kv_values.insert("bool".to_string(), KvsValue::from(true));
    kv_values.insert("str".to_string(), KvsValue::from("abcd".to_string()));
    kv_values.insert("null".to_string(), KvsValue::from(()));
    let hashmap = HashMap::from([("sub-number".to_string(), KvsValue::from(789.0))]);
    kv_values.insert("obj".to_string(), KvsValue::from(hashmap.clone()));
    let array = vec![
        KvsValue::from(321.0),
        KvsValue::from(false),
        KvsValue::from("dbca".to_string()),
        KvsValue::from(()),
        KvsValue::from(vec![]),
        KvsValue::from(hashmap),
    ];
    kv_values.insert("array".to_string(), KvsValue::from(array));

    {
        // First KVS run.
        let kvs = Kvs::open(
            InstanceId::new(0),
            OpenNeedDefaults::Optional,
            OpenNeedKvs::Optional,
            Some(dir_path.clone()),
        )?;
        kvs.flush_on_exit(false);

        // Set values.
        for (key, value) in kv_values.iter() {
            kvs.set_value(key, value.clone())?;
        }
    }

    // Assertions.
    {
        // Second KVS run.
        // KVS file is expected to not to exist.
        let kvs = Kvs::open(
            InstanceId::new(0),
            OpenNeedDefaults::Optional,
            OpenNeedKvs::Optional,
            Some(dir_path),
        )?;

        // Make sure no keys are defined.
        assert!(kvs.get_all_keys()?.is_empty());
    }

    Ok(())
}

#[test]
fn cit_persistency_flush_on_exit_disabled_manual_flush() -> Result<(), ErrorCode> {
    // Temp directory.
    let dir = tempdir()?;
    let dir_path = dir.path().to_string_lossy().to_string();

    // Values of each type.
    let mut kv_values: HashMap<String, KvsValue> = HashMap::new();
    kv_values.insert("number".to_string(), KvsValue::from(123.4));
    kv_values.insert("bool".to_string(), KvsValue::from(true));
    kv_values.insert("str".to_string(), KvsValue::from("abcd".to_string()));
    kv_values.insert("null".to_string(), KvsValue::from(()));
    let hashmap = HashMap::from([("sub-number".to_string(), KvsValue::from(789.0))]);
    kv_values.insert("obj".to_string(), KvsValue::from(hashmap.clone()));
    let array = vec![
        KvsValue::from(321.0),
        KvsValue::from(false),
        KvsValue::from("dbca".to_string()),
        KvsValue::from(()),
        KvsValue::from(vec![]),
        KvsValue::from(hashmap),
    ];
    kv_values.insert("array".to_string(), KvsValue::from(array));

    {
        // First KVS run.
        let kvs = Kvs::open(
            InstanceId::new(0),
            OpenNeedDefaults::Optional,
            OpenNeedKvs::Optional,
            Some(dir_path.clone()),
        )?;
        kvs.flush_on_exit(false);

        // Set values.
        for (key, value) in kv_values.iter() {
            kvs.set_value(key, value.clone())?;
        }

        // Explicitly flush.
        kvs.flush()?;
    }

    // Assertions.
    {
        // Second KVS run.
        // KVS file is expected to exist.
        let kvs = Kvs::open(
            InstanceId::new(0),
            OpenNeedDefaults::Optional,
            OpenNeedKvs::Required,
            Some(dir_path),
        )?;

        // Compare values.
        for (key, expected_value) in kv_values.iter() {
            let actual_value = kvs.get_value(&key).unwrap();
            assert!(compare_kvs_values(expected_value, &actual_value))
        }
    }

    Ok(())
}

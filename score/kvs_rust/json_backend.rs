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
use crate::error_code::ErrorCode;
use crate::kvs_api::{InstanceId, SnapshotId};
use crate::kvs_backend::KvsBackend;
use crate::kvs_value::{KvsMap, KvsValue};
use crate::log::{debug, error, trace, ScoreDebug};
use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};
use tinyjson::{JsonGenerateError, JsonParseError, JsonValue};

// Example of how KvsValue is stored in the JSON file (t-tagged format):
// {
//   "my_int": { "t": "i32", "v": 42 },
//   "my_float": { "t": "f64", "v": 3.1415 },
//   "my_bool": { "t": "bool", "v": true },
//   "my_string": { "t": "str", "v": "hello" },
//   "my_array": { "t": "arr", "v": [ ... ] },
//   "my_object": { "t": "obj", "v": { ... } },
//   "my_null": { "t": "null", "v": null }
// }

/// Backend-specific JsonValue -> KvsValue conversion.
impl From<JsonValue> for KvsValue {
    fn from(val: JsonValue) -> KvsValue {
        match val {
            JsonValue::Object(mut obj) => {
                // Type-tagged: { "t": ..., "v": ... }
                if let (Some(JsonValue::String(type_str)), Some(value)) = (obj.remove("t"), obj.remove("v")) {
                    return match (type_str.as_str(), value) {
                        ("i32", JsonValue::Number(v)) => KvsValue::I32(v as i32),
                        ("u32", JsonValue::Number(v)) => KvsValue::U32(v as u32),
                        ("i64", JsonValue::Number(v)) => KvsValue::I64(v as i64),
                        ("u64", JsonValue::Number(v)) => KvsValue::U64(v as u64),
                        ("f64", JsonValue::Number(v)) => KvsValue::F64(v),
                        ("bool", JsonValue::Boolean(v)) => KvsValue::Boolean(v),
                        ("str", JsonValue::String(v)) => KvsValue::String(v),
                        ("null", JsonValue::Null) => KvsValue::Null,
                        ("arr", JsonValue::Array(v)) => KvsValue::Array(v.into_iter().map(KvsValue::from).collect()),
                        ("obj", JsonValue::Object(v)) => {
                            KvsValue::Object(v.into_iter().map(|(k, v)| (k, KvsValue::from(v))).collect())
                        },
                        // Remaining types can be handled with Null.
                        _ => KvsValue::Null,
                    };
                }
                // If not a t-tagged object, treat as a map of key-value pairs (KvsMap)
                let map: KvsMap = obj.into_iter().map(|(k, v)| (k, KvsValue::from(v))).collect();
                KvsValue::Object(map)
            },
            // Remaining types can be handled with Null.
            _ => KvsValue::Null,
        }
    }
}

/// Backend-specific KvsValue -> JsonValue conversion.
impl From<KvsValue> for JsonValue {
    fn from(val: KvsValue) -> JsonValue {
        let mut obj = HashMap::new();
        match val {
            KvsValue::I32(n) => {
                obj.insert("t".to_string(), JsonValue::String("i32".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n as f64));
            },
            KvsValue::U32(n) => {
                obj.insert("t".to_string(), JsonValue::String("u32".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n as f64));
            },
            KvsValue::I64(n) => {
                obj.insert("t".to_string(), JsonValue::String("i64".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n as f64));
            },
            KvsValue::U64(n) => {
                obj.insert("t".to_string(), JsonValue::String("u64".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n as f64));
            },
            KvsValue::F64(n) => {
                obj.insert("t".to_string(), JsonValue::String("f64".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n));
            },
            KvsValue::Boolean(b) => {
                obj.insert("t".to_string(), JsonValue::String("bool".to_string()));
                obj.insert("v".to_string(), JsonValue::Boolean(b));
            },
            KvsValue::String(s) => {
                obj.insert("t".to_string(), JsonValue::String("str".to_string()));
                obj.insert("v".to_string(), JsonValue::String(s));
            },
            KvsValue::Null => {
                obj.insert("t".to_string(), JsonValue::String("null".to_string()));
                obj.insert("v".to_string(), JsonValue::Null);
            },
            KvsValue::Array(arr) => {
                obj.insert("t".to_string(), JsonValue::String("arr".to_string()));
                obj.insert(
                    "v".to_string(),
                    JsonValue::Array(arr.into_iter().map(JsonValue::from).collect()),
                );
            },
            KvsValue::Object(map) => {
                obj.insert("t".to_string(), JsonValue::String("obj".to_string()));
                obj.insert(
                    "v".to_string(),
                    JsonValue::Object(map.into_iter().map(|(k, v)| (k, JsonValue::from(v))).collect()),
                );
            },
        }
        JsonValue::Object(obj)
    }
}

/// tinyjson::JsonParseError -> ErrorCode::JsonParseError
impl From<JsonParseError> for ErrorCode {
    fn from(cause: JsonParseError) -> Self {
        error!(
            "JSON parser error: line = {}, column = {}",
            cause.line(),
            cause.column()
        );
        ErrorCode::JsonParserError
    }
}

/// tinyjson::JsonGenerateError -> ErrorCode::JsonGenerateError
impl From<JsonGenerateError> for ErrorCode {
    fn from(cause: JsonGenerateError) -> Self {
        error!("JSON generator error: msg = {}", cause.message());
        ErrorCode::JsonGeneratorError
    }
}

/// Builder for `JsonBackend`.
pub struct JsonBackendBuilder {
    working_dir: PathBuf,
    snapshot_max_count: usize,
}

impl JsonBackendBuilder {
    /// Create `JsonBackendBuilder`.
    ///
    /// Defaults:
    /// - `working_dir` - empty `PathBuf`, CWD is used.
    /// - `snapshot_max_count` - 3 snapshots.
    pub fn new() -> Self {
        Self {
            working_dir: PathBuf::new(),
            snapshot_max_count: 3,
        }
    }

    /// Set the working directory used by the JSON backend.
    pub fn working_dir(mut self, working_dir: PathBuf) -> Self {
        trace!("'working_dir' set to {:?}", working_dir);
        self.working_dir = working_dir;
        self
    }

    /// Set max number of snapshots.
    pub fn snapshot_max_count(mut self, snapshot_max_count: usize) -> Self {
        trace!("'snapshot_max_count' set to {:?}", snapshot_max_count);
        self.snapshot_max_count = snapshot_max_count;
        self
    }

    /// Finalize the builder and create JSON backend.
    pub fn build(self) -> JsonBackend {
        JsonBackend {
            working_dir: self.working_dir,
            snapshot_max_count: self.snapshot_max_count,
        }
    }
}

impl Default for JsonBackendBuilder {
    fn default() -> Self {
        Self::new()
    }
}

/// KVS backend implementation based on TinyJSON.
#[derive(Clone, Debug, PartialEq, ScoreDebug)]
pub struct JsonBackend {
    working_dir: PathBuf,
    snapshot_max_count: usize,
}

impl JsonBackend {
    fn parse(s: &str) -> Result<JsonValue, ErrorCode> {
        s.parse().map_err(ErrorCode::from)
    }

    fn stringify(val: &JsonValue) -> Result<String, ErrorCode> {
        val.stringify().map_err(ErrorCode::from)
    }

    /// Rotate snapshots
    ///
    /// # Features
    ///   * `FEAT_REQ__KVS__snapshots`
    ///
    /// # Return Values
    ///   * Ok: Rotation successful, also if no rotation was needed
    ///   * `ErrorCode::UnmappedError`: Unmapped error
    fn snapshot_rotate(&self, instance_id: InstanceId) -> Result<(), ErrorCode> {
        for idx in (1..self.snapshot_max_count()).rev() {
            let old_snapshot_id = SnapshotId(idx - 1);
            let new_snapshot_id = SnapshotId(idx);

            // Old paths.
            let hash_path_old = self.hash_file_path(instance_id, old_snapshot_id);
            let snap_name_old = Self::kvs_file_name(instance_id, old_snapshot_id);
            let snap_path_old = self.kvs_file_path(instance_id, old_snapshot_id);

            // Check snapshot and hash files exist.
            let snap_old_exists = snap_path_old.exists();
            let hash_old_exists = hash_path_old.exists();

            // Both files must exist to rotate.
            // If neither exist - continue.
            if !snap_old_exists && !hash_old_exists {
                continue;
            }
            // In other case - this is erroneous scenario.
            // Either snapshot or hash file got removed.
            else if !snap_old_exists || !hash_old_exists {
                error!("KVS or hash file not found");
                return Err(ErrorCode::IntegrityCorrupted);
            }

            // New paths.
            let hash_path_new = self.hash_file_path(instance_id, new_snapshot_id);
            let snap_name_new = Self::kvs_file_name(instance_id, new_snapshot_id);
            let snap_path_new = self.kvs_file_path(instance_id, new_snapshot_id);

            debug!("Rotating snapshots: {} -> {}", snap_name_old, snap_name_new);

            fs::rename(hash_path_old, hash_path_new)?;
            fs::rename(snap_path_old, snap_path_new)?;
        }

        Ok(())
    }

    /// Check path extensions are correct.
    fn check_path_extensions(kvs_path: &Path, hash_path: &Path) -> Result<(), ErrorCode> {
        fn check_extension(path: &Path, extension: &str) -> bool {
            let ext = path.extension();
            ext.is_some_and(|ep| ep.to_str().is_some_and(|es| es == extension))
        }

        debug!("Checking KVS file path: {:?}", kvs_path);
        if !check_extension(kvs_path, "json") {
            error!("Invalid KVS file path extension: {:?}", kvs_path);
            return Err(ErrorCode::KvsFileReadError);
        }

        debug!("Checking hash file path: {:?}", hash_path);
        if !check_extension(hash_path, "hash") {
            error!("Invalid hash file path extension: {:?}", hash_path);
            return Err(ErrorCode::KvsHashFileReadError);
        }

        Ok(())
    }

    pub(super) fn load(kvs_path: &Path, hash_path: &Path) -> Result<KvsMap, ErrorCode> {
        Self::check_path_extensions(kvs_path, hash_path)?;

        // Load KVS file.
        debug!("Loading KVS file: {:?}", kvs_path);
        let json_str = fs::read_to_string(kvs_path).inspect_err(|_| {
            error!("Failed to load KVS file: {:?}", kvs_path);
        })?;

        // Load hash file.
        debug!("Loading hash file: {:?}", hash_path);
        let hash_bytes = fs::read(hash_path).inspect_err(|_| {
            error!("Failed to load hash file: {:?}", hash_path);
        })?;

        // Perform hash check.
        debug!(
            "Performing hash check, KVS file: {:?}, hash file: {:?}",
            kvs_path, hash_path
        );
        if hash_bytes.len() != 4 {
            error!("Invalid hash length: {:?}", hash_path);
            return Err(ErrorCode::ValidationFailed);
        }

        let file_hash = u32::from_be_bytes([hash_bytes[0], hash_bytes[1], hash_bytes[2], hash_bytes[3]]);
        let hash_kvs = adler32::RollingAdler32::from_buffer(json_str.as_bytes()).hash();

        if hash_kvs != file_hash {
            error!("Hash mismatch, KVS file: {:?}, hash file: {:?}", kvs_path, hash_path);
            return Err(ErrorCode::ValidationFailed);
        }

        // Parse KVS from string to `JsonValue`.
        debug!("Parsing KVS file: {:?}", kvs_path);
        let json_value = Self::parse(&json_str).inspect_err(|_| {
            error!("Failed to parse KVS file: {:?}", kvs_path);
        })?;

        // Cast from `JsonValue` to `KvsValue`.
        debug!("Converting JSON values to KVS values");
        let kvs_value = KvsValue::from(json_value);
        if let KvsValue::Object(kvs_map) = kvs_value {
            Ok(kvs_map)
        } else {
            error!("Conversion from JSON to KVS failed");
            Err(ErrorCode::JsonParserError)
        }
    }

    pub(super) fn save(kvs_map: &KvsMap, kvs_path: &Path, hash_path: &Path) -> Result<(), ErrorCode> {
        Self::check_path_extensions(kvs_path, hash_path)?;

        // Cast from `KvsValue` to `JsonValue`.
        debug!("Converting KVS values to JSON values");
        let kvs_value = KvsValue::Object(kvs_map.clone());
        let json_value = JsonValue::from(kvs_value);

        // Stringify `JsonValue` and save to KVS file.
        debug!("Stringifying KVS file: {:?}", kvs_path);
        let json_str = Self::stringify(&json_value).inspect_err(|_| {
            error!("Failed to stringify KVS file content: {:?}", kvs_path);
        })?;

        debug!("Saving KVS file: {:?}", kvs_path);
        fs::write(kvs_path, &json_str).inspect_err(|_| {
            error!("Failed to save KVS file: {:?}", kvs_path);
        })?;

        // Generate hash and save to hash file.
        debug!(
            "Generating KVS hash, KVS file: {:?}, hash file: {:?}",
            kvs_path, hash_path
        );
        let hash = adler32::RollingAdler32::from_buffer(json_str.as_bytes()).hash();
        fs::write(hash_path, hash.to_be_bytes()).inspect_err(|_| {
            error!("Failed to save hash file: {:?}", hash_path);
        })?;

        Ok(())
    }

    /// Get KVS file name.
    pub fn kvs_file_name(instance_id: InstanceId, snapshot_id: SnapshotId) -> String {
        format!("kvs_{instance_id}_{snapshot_id}.json")
    }

    /// Get KVS file path in working directory.
    pub fn kvs_file_path(&self, instance_id: InstanceId, snapshot_id: SnapshotId) -> PathBuf {
        self.working_dir.join(Self::kvs_file_name(instance_id, snapshot_id))
    }

    /// Get hash file name.
    pub fn hash_file_name(instance_id: InstanceId, snapshot_id: SnapshotId) -> String {
        format!("kvs_{instance_id}_{snapshot_id}.hash")
    }

    /// Get hash file path in working directory.
    pub fn hash_file_path(&self, instance_id: InstanceId, snapshot_id: SnapshotId) -> PathBuf {
        self.working_dir.join(Self::hash_file_name(instance_id, snapshot_id))
    }

    /// Get defaults file name.
    pub fn defaults_file_name(instance_id: InstanceId) -> String {
        format!("kvs_{instance_id}_default.json")
    }

    /// Get defaults file path in working directory.
    pub fn defaults_file_path(&self, instance_id: InstanceId) -> PathBuf {
        self.working_dir.join(Self::defaults_file_name(instance_id))
    }

    /// Get defaults hash file name.
    pub fn defaults_hash_file_name(instance_id: InstanceId) -> String {
        format!("kvs_{instance_id}_default.hash")
    }

    /// Get defaults hash file path in working directory.
    pub fn defaults_hash_file_path(&self, instance_id: InstanceId) -> PathBuf {
        self.working_dir.join(Self::defaults_hash_file_name(instance_id))
    }
}

impl KvsBackend for JsonBackend {
    fn load_kvs(&self, instance_id: InstanceId, snapshot_id: SnapshotId) -> Result<KvsMap, ErrorCode> {
        let kvs_path = self.kvs_file_path(instance_id, snapshot_id);
        let hash_path = self.hash_file_path(instance_id, snapshot_id);
        Self::load(&kvs_path, &hash_path)
    }

    fn load_defaults(&self, instance_id: InstanceId) -> Result<KvsMap, ErrorCode> {
        let defaults_path = self.defaults_file_path(instance_id);
        let defaults_hash_path = self.defaults_hash_file_path(instance_id);
        Self::load(&defaults_path, &defaults_hash_path)
    }

    fn flush(&self, instance_id: InstanceId, kvs_map: &KvsMap) -> Result<(), ErrorCode> {
        self.snapshot_rotate(instance_id).inspect_err(|e| {
            error!("Failed to rotate snapshots: {:?}", e);
        })?;
        let snapshot_id = SnapshotId(0);
        let kvs_path = self.kvs_file_path(instance_id, snapshot_id);
        let hash_path = self.hash_file_path(instance_id, snapshot_id);
        Self::save(kvs_map, &kvs_path, &hash_path).inspect_err(|e| {
            error!("Failed to save snapshot: {:?}", e);
        })?;
        Ok(())
    }

    fn snapshot_count(&self, instance_id: InstanceId) -> usize {
        let mut count = 0;

        for idx in 0..self.snapshot_max_count {
            let snapshot_id = SnapshotId(idx);
            let snapshot_path = self.kvs_file_path(instance_id, snapshot_id);
            if !snapshot_path.exists() {
                break;
            }

            count += 1;
        }

        count
    }

    fn snapshot_max_count(&self) -> usize {
        self.snapshot_max_count
    }

    fn snapshot_restore(&self, instance_id: InstanceId, snapshot_id: SnapshotId) -> Result<KvsMap, ErrorCode> {
        // Fail if the snapshot ID is the current KVS.
        if snapshot_id == SnapshotId(0) {
            error!("Restoring current KVS snapshot is not allowed");
            return Err(ErrorCode::InvalidSnapshotId);
        }

        if self.snapshot_count(instance_id) < snapshot_id.0 {
            error!("Unable to restore non-existing snapshot");
            return Err(ErrorCode::InvalidSnapshotId);
        }

        self.load_kvs(instance_id, snapshot_id).inspect_err(|e| {
            error!("Failed to load snapshot: {:?}", e);
        })
    }
}

#[cfg(test)]
mod json_value_to_kvs_value_conversion_tests {
    use crate::kvs_value::{KvsMap, KvsValue};
    use std::collections::HashMap;
    use tinyjson::JsonValue;

    #[test]
    fn test_i32_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::I32(-123));
    }

    #[test]
    fn test_i32_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::String("-123.0".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_u32_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("u32".to_string())),
            ("v".to_string(), JsonValue::Number(123.0)),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::U32(123));
    }

    #[test]
    fn test_u32_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("u32".to_string())),
            ("v".to_string(), JsonValue::String("123.0".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_i64_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i64".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::I64(-123));
    }

    #[test]
    fn test_i64_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i64".to_string())),
            ("v".to_string(), JsonValue::String("-123.0".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_u64_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("u64".to_string())),
            ("v".to_string(), JsonValue::Number(123.0)),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::U64(123));
    }

    #[test]
    fn test_u64_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("u64".to_string())),
            ("v".to_string(), JsonValue::String("123.0".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_f64_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("f64".to_string())),
            ("v".to_string(), JsonValue::Number(-432.1)),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::F64(-432.1));
    }

    #[test]
    fn test_f64_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("f64".to_string())),
            ("v".to_string(), JsonValue::String("-432.1".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_bool_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("bool".to_string())),
            ("v".to_string(), JsonValue::Boolean(true)),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Boolean(true));
    }

    #[test]
    fn test_bool_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("bool".to_string())),
            ("v".to_string(), JsonValue::String("true".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_string_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("str".to_string())),
            ("v".to_string(), JsonValue::String("example".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::String("example".to_string()));
    }

    #[test]
    fn test_string_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("str".to_string())),
            ("v".to_string(), JsonValue::Number(123.4)),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_null_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("null".to_string())),
            ("v".to_string(), JsonValue::Null),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_null_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("null".to_string())),
            ("v".to_string(), JsonValue::Number(123.4)),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_array_ok() {
        let entry1 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let entry2 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("f64".to_string())),
            ("v".to_string(), JsonValue::Number(555.5)),
        ]));
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("arr".to_string())),
            ("v".to_string(), JsonValue::Array(vec![entry1, entry2])),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Array(vec![KvsValue::I32(-123), KvsValue::F64(555.5)]));
    }

    #[test]
    fn test_array_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("arr".to_string())),
            ("v".to_string(), JsonValue::String("example".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_object_ok() {
        let entry1 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let entry2 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("f64".to_string())),
            ("v".to_string(), JsonValue::Number(555.5)),
        ]));
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("obj".to_string())),
            (
                "v".to_string(),
                JsonValue::Object(HashMap::from([
                    ("entry1".to_string(), entry1.clone()),
                    ("entry2".to_string(), entry2.clone()),
                ])),
            ),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(
            kv,
            KvsValue::Object(KvsMap::from([
                ("entry1".to_string(), KvsValue::from(entry1)),
                ("entry2".to_string(), KvsValue::from(entry2))
            ]))
        );
    }

    #[test]
    fn test_object_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("obj".to_string())),
            ("v".to_string(), JsonValue::String("example".to_string())),
        ]));
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_non_json_value_object() {
        let jv = JsonValue::Number(123.0);
        let kv = KvsValue::from(jv);
        assert_eq!(kv, KvsValue::Null);
    }
}

#[cfg(test)]
mod kvs_value_to_json_value_conversion_tests {
    use crate::kvs_value::{KvsMap, KvsValue};
    use std::collections::HashMap;
    use tinyjson::JsonValue;

    #[test]
    fn test_i32_ok() {
        let kv = KvsValue::I32(-123);
        let jv = JsonValue::from(kv);

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("i32".to_string())),
                ("v".to_string(), JsonValue::Number(-123.0))
            ]))
        );
    }

    #[test]
    fn test_u32_ok() {
        let kv = KvsValue::U32(123);
        let jv = JsonValue::from(kv);

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("u32".to_string())),
                ("v".to_string(), JsonValue::Number(123.0))
            ]))
        );
    }

    #[test]
    fn test_i64_ok() {
        let kv = KvsValue::I64(-123);
        let jv = JsonValue::from(kv);

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("i64".to_string())),
                ("v".to_string(), JsonValue::Number(-123.0)),
            ]))
        );
    }

    #[test]
    fn test_u64_ok() {
        let kv = KvsValue::U64(123);
        let jv = JsonValue::from(kv);

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("u64".to_string())),
                ("v".to_string(), JsonValue::Number(123.0))
            ]))
        );
    }

    #[test]
    fn test_f64_ok() {
        let kv = KvsValue::F64(-432.1);
        let jv = JsonValue::from(kv);

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("f64".to_string())),
                ("v".to_string(), JsonValue::Number(-432.1)),
            ]))
        );
    }

    #[test]
    fn test_bool_ok() {
        let kv = KvsValue::Boolean(true);
        let jv = JsonValue::from(kv);

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("bool".to_string())),
                ("v".to_string(), JsonValue::Boolean(true)),
            ]))
        );
    }

    #[test]
    fn test_string_ok() {
        let kv = KvsValue::String("example".to_string());
        let jv = JsonValue::from(kv);

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("str".to_string())),
                ("v".to_string(), JsonValue::String("example".to_string())),
            ]))
        );
    }

    #[test]
    fn test_null_ok() {
        let kv = KvsValue::Null;
        let jv = JsonValue::from(kv);

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("null".to_string())),
                ("v".to_string(), JsonValue::Null),
            ]))
        );
    }

    #[test]
    fn test_array_ok() {
        let kv = KvsValue::Array(vec![KvsValue::I32(-123), KvsValue::F64(555.5)]);
        let jv = JsonValue::from(kv);

        let exp_entry1 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let exp_entry2 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("f64".to_string())),
            ("v".to_string(), JsonValue::Number(555.5)),
        ]));
        let exp_jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("arr".to_string())),
            ("v".to_string(), JsonValue::Array(vec![exp_entry1, exp_entry2])),
        ]));
        assert_eq!(jv, exp_jv);
    }

    #[test]
    fn test_object_ok() {
        let entry1 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let entry2 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("f64".to_string())),
            ("v".to_string(), JsonValue::Number(555.5)),
        ]));

        let kv = KvsValue::Object(KvsMap::from([
            ("entry1".to_string(), KvsValue::from(entry1.clone())),
            ("entry2".to_string(), KvsValue::from(entry2.clone())),
        ]));
        let jv = JsonValue::from(kv);

        let exp_jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("obj".to_string())),
            (
                "v".to_string(),
                JsonValue::Object(HashMap::from([
                    ("entry1".to_string(), entry1),
                    ("entry2".to_string(), entry2),
                ])),
            ),
        ]));
        assert_eq!(jv, exp_jv);
    }
}

#[cfg(test)]
mod error_code_tests {
    use crate::error_code::ErrorCode;
    use tinyjson::JsonValue;

    #[test]
    fn test_from_json_parse_error_to_json_parser_error() {
        let error = tinyjson::JsonParser::new("[1, 2, 3".chars()).parse().unwrap_err();
        assert_eq!(ErrorCode::from(error), ErrorCode::JsonParserError);
    }

    #[test]
    fn test_from_json_generate_error_to_json_generate_error() {
        let data: JsonValue = JsonValue::Number(f64::INFINITY);
        let error = data.stringify().unwrap_err();
        assert_eq!(ErrorCode::from(error), ErrorCode::JsonGeneratorError);
    }
}

#[cfg(test)]
mod json_backend_builder_tests {
    use crate::{json_backend::JsonBackendBuilder, prelude::KvsBackend};
    use std::path::PathBuf;
    use tempfile::tempdir;

    #[test]
    fn test_new_ok() {
        let builder = JsonBackendBuilder::new();

        // Assert builder params.
        assert_eq!(builder.working_dir, PathBuf::new());
        assert_eq!(builder.snapshot_max_count, 3);

        // Build and assert backend params.
        let backend = builder.build();
        assert_eq!(backend.working_dir, PathBuf::new());
        assert_eq!(backend.snapshot_max_count(), 3);
    }

    #[test]
    fn test_default_ok() {
        let builder = JsonBackendBuilder::default();

        // Assert builder params.
        assert_eq!(builder.working_dir, PathBuf::new());
        assert_eq!(builder.snapshot_max_count, 3);

        // Build and assert backend params.
        let backend = builder.build();
        assert_eq!(backend.working_dir, PathBuf::new());
        assert_eq!(backend.snapshot_max_count(), 3);
    }

    #[test]
    fn test_working_dir_ok() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let builder = JsonBackendBuilder::new().working_dir(dir_path.clone());

        // Assert builder params.
        assert_eq!(builder.working_dir, dir_path.clone());
        assert_eq!(builder.snapshot_max_count, 3);

        // Build and assert backend params.
        let backend = builder.build();
        assert_eq!(backend.working_dir, dir_path);
        assert_eq!(backend.snapshot_max_count(), 3);
    }

    #[test]
    fn test_snapshot_max_count_ok() {
        let builder = JsonBackendBuilder::new().snapshot_max_count(10);

        // Assert builder params.
        assert_eq!(builder.working_dir, PathBuf::new());
        assert_eq!(builder.snapshot_max_count, 10);

        // Build and assert backend params.
        let backend = builder.build();
        assert_eq!(backend.working_dir, PathBuf::new());
        assert_eq!(backend.snapshot_max_count(), 10);
    }

    #[test]
    fn test_chained_ok() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let builder = JsonBackendBuilder::new()
            .working_dir(dir_path.clone())
            .snapshot_max_count(10);

        // Assert builder params.
        assert_eq!(builder.working_dir, dir_path.clone());
        assert_eq!(builder.snapshot_max_count, 10);

        // Build and assert backend params.
        let backend = builder.build();
        assert_eq!(backend.working_dir, dir_path);
        assert_eq!(backend.snapshot_max_count(), 10);
    }
}

#[cfg(test)]
mod json_backend_tests {
    use crate::error_code::ErrorCode;
    use crate::json_backend::{JsonBackend, JsonBackendBuilder};
    use crate::kvs_api::{InstanceId, SnapshotId};
    use crate::kvs_value::{KvsMap, KvsValue};
    use std::path::{Path, PathBuf};
    use tempfile::tempdir;

    fn create_kvs_files(working_dir: &Path) -> (PathBuf, PathBuf) {
        let kvs_map = KvsMap::from([
            ("k1".to_string(), KvsValue::from("v1")),
            ("k2".to_string(), KvsValue::from(true)),
            ("k3".to_string(), KvsValue::from(123.4)),
        ]);
        let kvs_path = working_dir.join("kvs.json");
        let hash_path = working_dir.join("kvs.hash");
        JsonBackend::save(&kvs_map, &kvs_path, &hash_path).unwrap();
        (kvs_path, hash_path)
    }

    #[test]
    fn test_load_ok() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let (kvs_path, hash_path) = create_kvs_files(&dir_path);

        let kvs_map = JsonBackend::load(&kvs_path, &hash_path).unwrap();
        assert_eq!(kvs_map.len(), 3);
    }

    #[test]
    fn test_load_kvs_not_found() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let (kvs_path, hash_path) = create_kvs_files(&dir_path);
        std::fs::remove_file(&kvs_path).unwrap();

        assert!(JsonBackend::load(&kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::FileNotFound));
    }

    #[test]
    fn test_load_kvs_invalid_extension() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let kvs_path = dir_path.join("kvs.invalid_ext");
        let hash_path = dir_path.join("kvs.hash");

        assert!(JsonBackend::load(&kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::KvsFileReadError));
    }

    #[test]
    fn test_load_hash_not_found() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let (kvs_path, hash_path) = create_kvs_files(&dir_path);
        std::fs::remove_file(&hash_path).unwrap();

        assert!(JsonBackend::load(&kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::FileNotFound));
    }

    #[test]
    fn test_load_hash_invalid_extension() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let kvs_path = dir_path.join("kvs.json");
        let hash_path = dir_path.join("kvs.invalid_ext");

        assert!(JsonBackend::load(&kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::KvsHashFileReadError));
    }

    #[test]
    fn test_load_malformed_json() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let kvs_path = dir_path.join("kvs.json");
        let hash_path = dir_path.join("kvs.hash");

        let contents = "{\"malformed_json\"}";
        let hash = adler32::RollingAdler32::from_buffer(contents.as_bytes()).hash();
        std::fs::write(kvs_path.clone(), contents).unwrap();
        std::fs::write(hash_path.clone(), hash.to_be_bytes()).unwrap();

        assert!(JsonBackend::load(&kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::JsonParserError));
    }

    #[test]
    fn test_load_invalid_data() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let kvs_path = dir_path.join("kvs.json");
        let hash_path = dir_path.join("kvs.hash");

        let contents = "[123.4, 567.8]";
        let hash = adler32::RollingAdler32::from_buffer(contents.as_bytes()).hash();
        std::fs::write(kvs_path.clone(), contents).unwrap();
        std::fs::write(hash_path.clone(), hash.to_be_bytes()).unwrap();

        assert!(JsonBackend::load(&kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::JsonParserError));
    }

    #[test]
    fn test_load_invalid_hash_content() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let (kvs_path, hash_path) = create_kvs_files(&dir_path);
        std::fs::write(hash_path.clone(), vec![0x12, 0x34, 0x56, 0x78]).unwrap();

        assert!(JsonBackend::load(&kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::ValidationFailed));
    }

    #[test]
    fn test_load_invalid_hash_len() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let (kvs_path, hash_path) = create_kvs_files(&dir_path);
        std::fs::write(hash_path.clone(), vec![0x12, 0x34, 0x56]).unwrap();

        assert!(JsonBackend::load(&kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::ValidationFailed));
    }

    #[test]
    fn test_save_ok() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let kvs_map = KvsMap::from([
            ("k1".to_string(), KvsValue::from("v1")),
            ("k2".to_string(), KvsValue::from(true)),
            ("k3".to_string(), KvsValue::from(123.4)),
        ]);
        let kvs_path = dir_path.join("kvs.json");
        let hash_path = dir_path.join("kvs.hash");
        JsonBackend::save(&kvs_map, &kvs_path, &hash_path).unwrap();

        assert!(kvs_path.exists());
    }

    #[test]
    fn test_save_kvs_invalid_extension() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let kvs_map = KvsMap::new();
        let kvs_path = dir_path.join("kvs.invalid_ext");
        let hash_path = dir_path.join("kvs.hash");

        assert!(JsonBackend::save(&kvs_map, &kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::KvsFileReadError));
    }

    #[test]
    fn test_save_hash_invalid_extension() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let kvs_map = KvsMap::new();
        let kvs_path = dir_path.join("kvs.json");
        let hash_path = dir_path.join("kvs.invalid_ext");

        assert!(JsonBackend::save(&kvs_map, &kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::KvsHashFileReadError));
    }

    #[test]
    fn test_save_impossible_str() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let kvs_map = KvsMap::from([("inf".to_string(), KvsValue::from(f64::INFINITY))]);
        let kvs_path = dir_path.join("kvs.json");
        let hash_path = dir_path.join("kvs.hash");

        assert!(JsonBackend::save(&kvs_map, &kvs_path, &hash_path).is_err_and(|e| e == ErrorCode::JsonGeneratorError));
    }

    #[test]
    fn test_kvs_file_name() {
        let instance_id = InstanceId(123);
        let snapshot_id = SnapshotId(2);
        let exp_name = format!("kvs_{instance_id}_{snapshot_id}.json");
        let act_name = JsonBackend::kvs_file_name(instance_id, snapshot_id);
        assert_eq!(exp_name, act_name);
    }

    #[test]
    fn test_kvs_file_path() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();

        let instance_id = InstanceId(123);
        let snapshot_id = SnapshotId(2);
        let exp_name = dir_path.join(format!("kvs_{instance_id}_{snapshot_id}.json"));
        let act_name = backend.kvs_file_path(instance_id, snapshot_id);
        assert_eq!(exp_name, act_name);
    }
    #[test]
    fn test_hash_file_name() {
        let instance_id = InstanceId(123);
        let snapshot_id = SnapshotId(2);
        let exp_name = format!("kvs_{instance_id}_{snapshot_id}.hash");
        let act_name = JsonBackend::hash_file_name(instance_id, snapshot_id);
        assert_eq!(exp_name, act_name);
    }

    #[test]
    fn test_hash_file_path() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();

        let instance_id = InstanceId(123);
        let snapshot_id = SnapshotId(2);
        let exp_name = dir_path.join(format!("kvs_{instance_id}_{snapshot_id}.hash"));
        let act_name = backend.hash_file_path(instance_id, snapshot_id);
        assert_eq!(exp_name, act_name);
    }

    #[test]
    fn test_defaults_file_name() {
        let instance_id = InstanceId(123);
        let exp_name = format!("kvs_{instance_id}_default.json");
        let act_name = JsonBackend::defaults_file_name(instance_id);
        assert_eq!(exp_name, act_name);
    }

    #[test]
    fn test_defaults_file_path() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();

        let instance_id = InstanceId(123);
        let exp_name = dir_path.join(format!("kvs_{instance_id}_default.json"));
        let act_name = backend.defaults_file_path(instance_id);
        assert_eq!(exp_name, act_name);
    }

    #[test]
    fn test_defaults_hash_file_name() {
        let instance_id = InstanceId(123);
        let exp_name = format!("kvs_{instance_id}_default.hash");
        let act_name = JsonBackend::defaults_hash_file_name(instance_id);
        assert_eq!(exp_name, act_name);
    }

    #[test]
    fn test_defaults_hash_file_path() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();

        let instance_id = InstanceId(123);
        let exp_name = dir_path.join(format!("kvs_{instance_id}_default.hash"));
        let act_name = backend.defaults_hash_file_path(instance_id);
        assert_eq!(exp_name, act_name);
    }
}

#[cfg(test)]
mod kvs_backend_tests {
    use crate::error_code::ErrorCode;
    use crate::json_backend::{JsonBackend, JsonBackendBuilder};
    use crate::kvs_api::{InstanceId, SnapshotId};
    use crate::kvs_backend::KvsBackend;
    use crate::kvs_value::{KvsMap, KvsValue};
    use std::fs;
    use tempfile::tempdir;

    fn create_kvs_files(backend: &JsonBackend, instance_id: InstanceId, snapshot_id: SnapshotId) {
        let kvs_map = KvsMap::from([
            ("k1".to_string(), KvsValue::from("v1")),
            ("k2".to_string(), KvsValue::from(true)),
            ("k3".to_string(), KvsValue::from(123.4)),
        ]);
        let kvs_path = backend.kvs_file_path(instance_id, snapshot_id);
        let hash_path = backend.hash_file_path(instance_id, snapshot_id);
        JsonBackend::save(&kvs_map, &kvs_path, &hash_path).unwrap();
    }

    fn create_defaults_file(backend: &JsonBackend, instance_id: InstanceId) {
        let kvs_map = KvsMap::from([
            ("k4".to_string(), KvsValue::from("v4")),
            ("k5".to_string(), KvsValue::from(432.1)),
        ]);
        let defaults_path = backend.defaults_file_path(instance_id);
        let defaults_hash_path = backend.defaults_hash_file_path(instance_id);
        JsonBackend::save(&kvs_map, &defaults_path, &defaults_hash_path).unwrap();
    }

    #[test]
    fn test_load_kvs_ok() {
        // Main `load` tests are performed by `test_load_*` tests.
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(1);
        let snapshot_id = SnapshotId(1);
        create_kvs_files(&backend, instance_id, snapshot_id);

        let kvs_map = backend.load_kvs(instance_id, snapshot_id).unwrap();
        assert_eq!(kvs_map.len(), 3);
    }

    #[test]
    fn test_load_defaults_ok() {
        // Main `load` tests are performed by `test_load_*` tests.
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(1);
        create_defaults_file(&backend, instance_id);

        let kvs_map = backend.load_defaults(instance_id).unwrap();
        assert_eq!(kvs_map.len(), 2);
    }

    #[test]
    fn test_flush_ok() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(1);

        // Flush.
        let kvs_map = KvsMap::from([("key".to_string(), KvsValue::from("value"))]);
        backend.flush(instance_id, &kvs_map).unwrap();

        // Check files exist.
        let snapshot_id = SnapshotId(0);
        let kvs_path = backend.kvs_file_path(instance_id, snapshot_id);
        let hash_path = backend.hash_file_path(instance_id, snapshot_id);
        assert!(kvs_path.exists());
        assert!(hash_path.exists());
    }

    #[test]
    fn test_flush_kvs_removed() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(1);

        // Flush.
        let kvs_map = KvsMap::from([("key".to_string(), KvsValue::from("value"))]);
        backend.flush(instance_id, &kvs_map).unwrap();

        // Remove KVS file.
        let snapshot_id = SnapshotId(0);
        let kvs_path = backend.kvs_file_path(instance_id, snapshot_id);
        fs::remove_file(kvs_path).unwrap();

        // Flush again.
        let result = backend.flush(instance_id, &kvs_map);
        assert!(result.is_err_and(|e| e == ErrorCode::IntegrityCorrupted));
    }

    #[test]
    fn test_flush_hash_removed() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(1);

        // Flush.
        let kvs_map = KvsMap::from([("key".to_string(), KvsValue::from("value"))]);
        backend.flush(instance_id, &kvs_map).unwrap();

        // Remove KVS file.
        let snapshot_id = SnapshotId(0);
        let hash_path = backend.hash_file_path(instance_id, snapshot_id);
        fs::remove_file(hash_path).unwrap();

        // Flush again.
        let result = backend.flush(instance_id, &kvs_map);
        assert!(result.is_err_and(|e| e == ErrorCode::IntegrityCorrupted));
    }

    #[test]
    fn test_snapshot_count_zero() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(2);

        assert_eq!(backend.snapshot_count(instance_id), 0);
    }

    #[test]
    fn test_snapshot_count_to_one() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(2);

        backend.flush(instance_id, &KvsMap::new()).unwrap();
        assert_eq!(backend.snapshot_count(instance_id), 1);
    }

    #[test]
    fn test_snapshot_count_to_max() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(2);

        for i in 1..=backend.snapshot_max_count() {
            backend.flush(instance_id, &KvsMap::new()).unwrap();
            assert_eq!(backend.snapshot_count(instance_id), i);
        }

        backend.flush(instance_id, &KvsMap::new()).unwrap();
        backend.flush(instance_id, &KvsMap::new()).unwrap();
        assert_eq!(backend.snapshot_count(instance_id), backend.snapshot_max_count());
    }

    #[test]
    fn test_snapshot_max_count() {
        let max_count = 1234;
        let backend = JsonBackendBuilder::new().snapshot_max_count(max_count).build();
        assert_eq!(backend.snapshot_max_count(), max_count);
    }

    #[test]
    fn test_snapshot_restore_ok() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(2);

        // Prepare snapshots.
        for i in 1..=backend.snapshot_max_count() {
            let kvs_map = KvsMap::from([("counter".to_string(), KvsValue::I32(i as i32))]);
            backend.flush(instance_id, &kvs_map).unwrap();
        }

        // Check restore.
        let kvs_map = backend.snapshot_restore(instance_id, SnapshotId(1)).unwrap();
        assert_eq!(*kvs_map.get("counter").unwrap(), KvsValue::I32(2));
    }

    #[test]
    fn test_snapshot_restore_invalid_id() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(2);

        // Prepare snapshots.
        for i in 1..=backend.snapshot_max_count() {
            let kvs_map = KvsMap::from([("counter".to_string(), KvsValue::I32(i as i32))]);
            backend.flush(instance_id, &kvs_map).unwrap();
        }

        let result = backend.snapshot_restore(instance_id, SnapshotId(123));
        assert!(result.is_err_and(|e| e == ErrorCode::InvalidSnapshotId));
    }

    #[test]
    fn test_snapshot_restore_current_id() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(2);

        // Prepare snapshots.
        for i in 1..=backend.snapshot_max_count() {
            let kvs_map = KvsMap::from([("counter".to_string(), KvsValue::I32(i as i32))]);
            backend.flush(instance_id, &kvs_map).unwrap();
        }

        let result = backend.snapshot_restore(instance_id, SnapshotId(0));
        assert!(result.is_err_and(|e| e == ErrorCode::InvalidSnapshotId));
    }
}

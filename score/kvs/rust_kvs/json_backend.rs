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
use crate::kvs_api::{is_key_valid, InstanceId, SnapshotId};
use crate::kvs_backend::KvsBackend;
use crate::kvs_value::{KvsMap, KvsValue};
use crate::log::{debug, error, trace, ScoreDebug};
use std::collections::HashMap;
use std::fs;
use std::io::Write;
use std::os::unix::fs::{OpenOptionsExt, PermissionsExt};
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

/// Integer types stored as JSON numbers.
trait JsonInteger: Sized {
    /// Converts JSON number to `Self`.
    /// Number must be equal to a value of `Self` converted to `f64`.
    fn from_json_number(value: f64) -> Option<Self>;
}

macro_rules! impl_json_integer {
    ($($t:ty),*) => {
        $(impl JsonInteger for $t {
            fn from_json_number(value: f64) -> Option<Self> {
                // `as` saturates, so fractional, non-finite and out-of-range values don't round-trip.
                // 64-bit `MAX` is written rounded up (e.g., 2^64), which saturates back to `MAX`.
                let converted = value as $t;
                (converted as f64 == value).then_some(converted)
            }
        })*
    };
}

impl_json_integer!(u8, u16, u32, u64, i8, i16, i32, i64);

/// Converts JSON number to integer type `T`.
fn checked_integer<T: JsonInteger>(value: f64) -> Result<T, ErrorCode> {
    T::from_json_number(value).ok_or(ErrorCode::InvalidValueType)
}

/// Converts JSON number to `f32`.
/// Number must round to a finite `f32` value.
/// `f32::MAX` written by C++ can parse to `f64` slightly above it, which still rounds back to it.
fn checked_f32(value: f64) -> Result<f32, ErrorCode> {
    let converted = value as f32;
    if converted.is_finite() {
        Ok(converted)
    } else {
        Err(ErrorCode::InvalidValueType)
    }
}

/// Backend-specific JsonValue -> KvsValue conversion.
impl TryFrom<JsonValue> for KvsValue {
    type Error = ErrorCode;

    fn try_from(value: JsonValue) -> Result<Self, Self::Error> {
        match value {
            JsonValue::Object(mut obj) => {
                // Type-tagged: { "t": ..., "v": ... }
                if let (Some(JsonValue::String(type_str)), Some(value)) = (obj.remove("t"), obj.remove("v")) {
                    match (type_str.as_str(), value) {
                        ("u8", JsonValue::Number(v)) => checked_integer(v).map(KvsValue::U8),
                        ("u16", JsonValue::Number(v)) => checked_integer(v).map(KvsValue::U16),
                        ("u32", JsonValue::Number(v)) => checked_integer(v).map(KvsValue::U32),
                        ("u64", JsonValue::Number(v)) => checked_integer(v).map(KvsValue::U64),
                        ("i8", JsonValue::Number(v)) => checked_integer(v).map(KvsValue::I8),
                        ("i16", JsonValue::Number(v)) => checked_integer(v).map(KvsValue::I16),
                        ("i32", JsonValue::Number(v)) => checked_integer(v).map(KvsValue::I32),
                        ("i64", JsonValue::Number(v)) => checked_integer(v).map(KvsValue::I64),
                        ("f32", JsonValue::Number(v)) => checked_f32(v).map(KvsValue::F32),
                        ("f64", JsonValue::Number(v)) if v.is_finite() => Ok(KvsValue::F64(v)),
                        ("bool", JsonValue::Boolean(v)) => Ok(KvsValue::Boolean(v)),
                        ("str", JsonValue::String(v)) => Ok(KvsValue::String(v)),
                        ("null", JsonValue::Null) => Ok(KvsValue::Null),
                        ("arr", JsonValue::Array(v)) => {
                            let converted_result = v.into_iter().map(KvsValue::try_from).collect::<Result<Vec<_>, _>>();
                            Ok(KvsValue::Array(converted_result?))
                        },
                        ("obj", JsonValue::Object(v)) => {
                            let converted_result = v
                                .into_iter()
                                .map(|(k, v)| KvsValue::try_from(v).map(|v| (k, v)))
                                .collect::<Result<KvsMap, _>>();
                            Ok(KvsValue::Object(converted_result?))
                        },
                        _ => Err(ErrorCode::InvalidValueType),
                    }
                } else {
                    Err(ErrorCode::ConversionFailed)
                }
            },
            _ => Err(ErrorCode::ConversionFailed),
        }
    }
}

/// Backend-specific KvsValue -> JsonValue conversion.
impl TryFrom<KvsValue> for JsonValue {
    type Error = ErrorCode;

    fn try_from(val: KvsValue) -> Result<Self, Self::Error> {
        // TinyJSON holds every number as `f64`, so `u64` and `i64` values beyond 2^53 are rounded.
        let (type_tag, value) = match val {
            KvsValue::U8(n) => ("u8", JsonValue::Number(n as f64)),
            KvsValue::U16(n) => ("u16", JsonValue::Number(n as f64)),
            KvsValue::U32(n) => ("u32", JsonValue::Number(n as f64)),
            KvsValue::U64(n) => ("u64", JsonValue::Number(n as f64)),
            KvsValue::I8(n) => ("i8", JsonValue::Number(n as f64)),
            KvsValue::I16(n) => ("i16", JsonValue::Number(n as f64)),
            KvsValue::I32(n) => ("i32", JsonValue::Number(n as f64)),
            KvsValue::I64(n) => ("i64", JsonValue::Number(n as f64)),
            KvsValue::F32(n) => ("f32", JsonValue::Number(n as f64)),
            KvsValue::F64(n) => ("f64", JsonValue::Number(n)),
            KvsValue::Boolean(b) => ("bool", JsonValue::Boolean(b)),
            KvsValue::String(s) => ("str", JsonValue::String(s)),
            KvsValue::Null => ("null", JsonValue::Null),
            KvsValue::Array(arr) => (
                "arr",
                JsonValue::Array(
                    arr.into_iter()
                        .map(JsonValue::try_from)
                        .collect::<Result<Vec<_>, _>>()?,
                ),
            ),
            KvsValue::Object(map) => (
                "obj",
                JsonValue::Object(
                    map.into_iter()
                        .map(|(k, v)| JsonValue::try_from(v).map(|v| (k, v)))
                        .collect::<Result<HashMap<_, _>, _>>()?,
                ),
            ),
        };
        Ok(JsonValue::Object(HashMap::from([
            ("t".to_string(), JsonValue::String(type_tag.to_string())),
            ("v".to_string(), value),
        ])))
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
        trace!("'working_dir' set to {}", working_dir.to_string_lossy().as_ref());
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
#[derive(Clone, Debug, PartialEq)]
pub struct JsonBackend {
    working_dir: PathBuf,
    snapshot_max_count: usize,
}

impl ScoreDebug for JsonBackend {
    fn fmt(&self, f: score_log::fmt::Writer, spec: &score_log::fmt::FormatSpec) -> score_log::fmt::Result {
        score_log::fmt::DebugStruct::new(f, spec, "JsonBackend")
            .field("working_dir", &self.working_dir.to_string_lossy().as_ref())
            .field("snapshot_max_count", &self.snapshot_max_count)
            .finish()
    }
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
                error!(
                    "KVS or hash file not found: instance={}, snapshot={}",
                    instance_id, old_snapshot_id
                );
                return Err(ErrorCode::IntegrityCorrupted);
            }

            // New paths.
            let hash_path_new = self.hash_file_path(instance_id, new_snapshot_id);
            let snap_path_new = self.kvs_file_path(instance_id, new_snapshot_id);

            debug!(
                "Rotating snapshots: instance={}, snapshot={} -> {}",
                instance_id, old_snapshot_id, new_snapshot_id
            );

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

        let kvs_path_str: &str = &kvs_path.to_string_lossy();
        let hash_path_str: &str = &hash_path.to_string_lossy();

        debug!("Checking KVS file extension: {}", kvs_path_str);
        if !check_extension(kvs_path, "json") {
            error!("Invalid KVS file extension: {}", kvs_path_str);
            return Err(ErrorCode::KvsFileReadError);
        }

        debug!("Checking hash file extension: {}", hash_path_str);
        if !check_extension(hash_path, "hash") {
            error!("Invalid hash file extension: {}", hash_path_str);
            return Err(ErrorCode::KvsHashFileReadError);
        }

        Ok(())
    }

    pub(super) fn load(kvs_path: &Path, hash_path: &Path) -> Result<KvsMap, ErrorCode> {
        let kvs_path_str: &str = &kvs_path.to_string_lossy();
        let hash_path_str: &str = &hash_path.to_string_lossy();
        Self::check_path_extensions(kvs_path, hash_path)?;

        // Load KVS file.
        debug!("Loading KVS file: {}", kvs_path_str);
        let json_str = fs::read_to_string(kvs_path).inspect_err(|_| {
            error!("Failed to load KVS file: {}", kvs_path_str);
        })?;

        // Load hash file.
        debug!("Loading hash file: {}", hash_path_str);
        let hash_bytes = fs::read(hash_path).inspect_err(|_| {
            error!("Failed to load hash file: {}", hash_path_str);
        })?;

        // Perform hash check.
        debug!(
            "Performing KVS hash check, KVS file: {}, hash file: {}",
            kvs_path_str, hash_path_str
        );
        if hash_bytes.len() != 4 {
            error!("Invalid hash length: {}", hash_path_str);
            return Err(ErrorCode::ValidationFailed);
        }

        let file_hash = u32::from_be_bytes([hash_bytes[0], hash_bytes[1], hash_bytes[2], hash_bytes[3]]);
        let hash_kvs = adler32::RollingAdler32::from_buffer(json_str.as_bytes()).hash();

        if hash_kvs != file_hash {
            error!(
                "KVS hash mismatch, KVS file: {}, hash file: {}",
                kvs_path_str, hash_path_str
            );
            return Err(ErrorCode::ValidationFailed);
        }

        // Parse KVS from string to `JsonValue`.
        debug!("Parsing KVS file: {}", kvs_path_str);
        let json_value = Self::parse(&json_str).inspect_err(|_| {
            error!("Failed to parse KVS file: {}", kvs_path_str);
        })?;

        // Cast from `JsonValue` to `KvsValue`.
        debug!("Converting JSON values to KVS values: {}", kvs_path_str);
        let kvs_value = KvsValue::try_from(json_value)?;
        if let KvsValue::Object(kvs_map) = kvs_value {
            // Validate keys.
            if let Some(key) = kvs_map.keys().find(|key| !is_key_valid(key)) {
                error!("Provided key is not valid: {}, KVS file: {}", key, kvs_path_str);
                return Err(ErrorCode::InvalidKey);
            }

            Ok(kvs_map)
        } else {
            error!("Conversion from JSON to KVS failed: {}", kvs_path_str);
            Err(ErrorCode::JsonParserError)
        }
    }

    /// Get path of the temporary file a KVS file is written to before it replaces the KVS file.
    fn temp_file_path(path: &Path) -> PathBuf {
        let mut tmp_path = path.as_os_str().to_owned();
        tmp_path.push(".tmp");
        PathBuf::from(tmp_path)
    }

    /// Write data to the temporary file of `path`, set its permissions to 640 and request the OS to commit it to physical storage.
    /// File at `path` is not modified, it is replaced by `commit_temp_file`.
    fn write_temp_file(path: &Path, data: &[u8]) -> Result<PathBuf, ErrorCode> {
        const KVS_FILE_MODE: u32 = 0o640;
        let tmp_path = Self::temp_file_path(path);
        let tmp_path_str: &str = &tmp_path.to_string_lossy();

        // Remove temporary file left behind by an interrupted write.
        if let Err(e) = fs::remove_file(&tmp_path) {
            if e.kind() != std::io::ErrorKind::NotFound {
                error!(
                    "Failed to remove stale temporary file: {}, error: {:?}",
                    tmp_path_str,
                    e.to_string()
                );
                return Err(ErrorCode::PhysicalStorageFailure);
            }
        }

        // Create new file.
        // Mode is applied on creation and file planted at the path is never opened.
        let mut file = fs::OpenOptions::new()
            .write(true)
            .create_new(true)
            .mode(KVS_FILE_MODE)
            .open(&tmp_path)
            .map_err(|e| {
                error!(
                    "Failed to open temporary file: {}, error: {:?}",
                    tmp_path_str,
                    e.to_string()
                );
                ErrorCode::PhysicalStorageFailure
            })?;

        let result = (|| {
            // Set permissions on a file already opened, mode provided on creation is restricted by umask.
            // `fchmod` is not supported by Miri.
            #[cfg(not(miri))]
            file.set_permissions(fs::Permissions::from_mode(KVS_FILE_MODE))
                .map_err(|_| {
                    error!("Failed to set temporary file permissions: {}", tmp_path_str);
                    ErrorCode::PhysicalStorageFailure
                })?;

            file.write_all(data).map_err(|e| {
                error!(
                    "Failed to write to temporary file: {}, error: {:?}",
                    tmp_path_str,
                    e.to_string()
                );
                ErrorCode::PhysicalStorageFailure
            })?;
            file.sync_data().map_err(|e| {
                error!(
                    "Failed to sync temporary file: {}, error: {:?}",
                    tmp_path_str,
                    e.to_string()
                );
                ErrorCode::PhysicalStorageFailure
            })
        })();
        drop(file);

        // Remove temporary file on failure.
        if let Err(e) = result {
            let _ = fs::remove_file(&tmp_path);
            return Err(e);
        }

        Ok(tmp_path)
    }

    /// Replace file at `path` with its temporary file.
    /// Rename is atomic - file is either old or new.
    fn commit_temp_file(tmp_path: &Path, path: &Path) -> Result<(), ErrorCode> {
        fs::rename(tmp_path, path).map_err(|e| {
            error!(
                "Failed to replace file: {}, error: {:?}",
                path.to_string_lossy().as_ref(),
                e.to_string()
            );
            let _ = fs::remove_file(tmp_path);
            ErrorCode::PhysicalStorageFailure
        })
    }

    /// Request the OS to commit entries of the directory containing `path` to physical storage.
    fn sync_parent_dir(path: &Path) -> Result<(), ErrorCode> {
        let dir = match path.parent() {
            Some(dir) if !dir.as_os_str().is_empty() => dir,
            _ => Path::new("."),
        };
        fs::File::open(dir)
            .and_then(|dir_file| dir_file.sync_all())
            .map_err(|e| {
                error!(
                    "Failed to sync parent directory: {}, error: {:?}",
                    dir.to_string_lossy().as_ref(),
                    e.to_string()
                );
                ErrorCode::PhysicalStorageFailure
            })
    }

    pub(super) fn save(kvs_map: &KvsMap, kvs_path: &Path, hash_path: &Path) -> Result<(), ErrorCode> {
        let kvs_path_str: &str = &kvs_path.to_string_lossy();
        let hash_path_str: &str = &hash_path.to_string_lossy();
        Self::check_path_extensions(kvs_path, hash_path)?;

        // Cast from `KvsValue` to `JsonValue`.
        debug!("Converting KVS values to JSON values: {}", kvs_path_str);
        let kvs_value = KvsValue::Object(kvs_map.clone());
        let json_value = JsonValue::try_from(kvs_value)?;

        // Stringify `JsonValue` and save to temporary KVS file.
        debug!("Stringifying KVS file: {}", kvs_path_str);
        let json_str = Self::stringify(&json_value).inspect_err(|_| {
            error!("Failed to stringify KVS file content: {}", kvs_path_str);
        })?;

        debug!("Saving KVS file: {}", kvs_path_str);
        let kvs_tmp_path = Self::write_temp_file(kvs_path, json_str.as_bytes()).inspect_err(|_| {
            error!("Failed to save KVS file: {}", kvs_path_str);
        })?;

        // Generate hash and save to temporary hash file.
        debug!(
            "Generating KVS hash, KVS file: {}, hash file: {}",
            kvs_path_str, hash_path_str
        );
        let hash = adler32::RollingAdler32::from_buffer(json_str.as_bytes()).hash();
        let hash_tmp_path = Self::write_temp_file(hash_path, &hash.to_be_bytes()).inspect_err(|_| {
            error!("Failed to save hash file: {}", hash_path_str);
            let _ = fs::remove_file(&kvs_tmp_path);
        })?;

        // Replace KVS and hash files with temporary files.
        Self::commit_temp_file(&kvs_tmp_path, kvs_path).inspect_err(|_| {
            let _ = fs::remove_file(&hash_tmp_path);
        })?;
        Self::commit_temp_file(&hash_tmp_path, hash_path)?;
        Self::sync_parent_dir(kvs_path)
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
        debug!("Loading snapshot: instance={}, snapshot={}", instance_id, snapshot_id);
        let kvs_path = self.kvs_file_path(instance_id, snapshot_id);
        let hash_path = self.hash_file_path(instance_id, snapshot_id);
        Self::load(&kvs_path, &hash_path).inspect_err(|e| {
            error!(
                "Failed to load snapshot: instance={}, snapshot={}, error={:?}",
                instance_id, snapshot_id, e
            );
        })
    }

    fn load_defaults(&self, instance_id: InstanceId) -> Result<KvsMap, ErrorCode> {
        debug!("Loading defaults: instance={}", instance_id);
        let defaults_path = self.defaults_file_path(instance_id);
        let defaults_hash_path = self.defaults_hash_file_path(instance_id);
        Self::load(&defaults_path, &defaults_hash_path).inspect_err(|e| {
            error!("Failed to load defaults: instance={}, error={:?}", instance_id, e);
        })
    }

    fn flush(&self, instance_id: InstanceId, kvs_map: &KvsMap) -> Result<(), ErrorCode> {
        self.snapshot_rotate(instance_id).inspect_err(|e| {
            error!("Failed to rotate snapshots: instance={}, error={:?}", instance_id, e);
        })?;
        let snapshot_id = SnapshotId(0);
        debug!("Saving snapshot: instance={}, snapshot={}", instance_id, snapshot_id);
        let kvs_path = self.kvs_file_path(instance_id, snapshot_id);
        let hash_path = self.hash_file_path(instance_id, snapshot_id);
        Self::save(kvs_map, &kvs_path, &hash_path).inspect_err(|e| {
            error!(
                "Failed to save snapshot: instance={}, snapshot={}, error={:?}",
                instance_id, snapshot_id, e
            );
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
            error!(
                "Restoring current KVS snapshot is not allowed: instance={}",
                instance_id
            );
            return Err(ErrorCode::InvalidSnapshotId);
        }

        if self.snapshot_count(instance_id) < snapshot_id.0 {
            error!(
                "Unable to restore non-existing snapshot: instance={}, snapshot={}",
                instance_id, snapshot_id
            );
            return Err(ErrorCode::InvalidSnapshotId);
        }

        self.load_kvs(instance_id, snapshot_id)
    }
}

#[cfg(test)]
mod json_value_to_kvs_value_conversion_tests {
    use crate::error_code::ErrorCode;
    use crate::kvs_value::{KvsMap, KvsValue};
    use std::collections::HashMap;
    use tinyjson::JsonValue;

    fn tagged(tag: &str, value: JsonValue) -> JsonValue {
        JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String(tag.to_string())),
            ("v".to_string(), value),
        ]))
    }

    /// Exact power of two.
    /// Precision of `powi` is unspecified and Miri randomizes it.
    fn pow2(exp: i32) -> f64 {
        f64::from_bits(((1023 + exp) as u64) << 52)
    }

    fn assert_number_ok(tag: &str, value: f64, expected: KvsValue) {
        assert_eq!(
            KvsValue::try_from(tagged(tag, JsonValue::Number(value))),
            Ok(expected),
            "{tag}: {value}"
        );
    }

    fn assert_number_invalid(tag: &str, value: f64) {
        assert_eq!(
            KvsValue::try_from(tagged(tag, JsonValue::Number(value))),
            Err(ErrorCode::InvalidValueType),
            "{tag}: {value}"
        );
    }

    macro_rules! integer_range_tests {
        ($mod_name:ident, $t:ty, $tag:literal, $variant:ident) => {
            mod $mod_name {
                use super::*;

                #[test]
                fn test_in_range_ok() {
                    for value in [<$t>::MIN, <$t>::MIN / 3, 0, 1, <$t>::MAX / 3, <$t>::MAX] {
                        assert_number_ok($tag, value as f64, KvsValue::$variant(value));
                    }
                }

                #[test]
                fn test_out_of_range() {
                    assert_number_invalid($tag, <$t>::MIN as f64 - 1.0);
                    assert_number_invalid($tag, <$t>::MAX as f64 + 1.0);
                }

                #[test]
                fn test_fractional() {
                    assert_number_invalid($tag, 1.5);
                    assert_number_invalid($tag, (<$t>::MAX / 3) as f64 + 0.5);
                }

                #[test]
                fn test_non_finite() {
                    assert_number_invalid($tag, f64::NAN);
                    assert_number_invalid($tag, f64::INFINITY);
                    assert_number_invalid($tag, f64::NEG_INFINITY);
                }

                #[test]
                fn test_invalid_type() {
                    let jv = tagged($tag, JsonValue::String("1".to_string()));
                    assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
                }
            }
        };
    }

    integer_range_tests!(u8_tests, u8, "u8", U8);
    integer_range_tests!(u16_tests, u16, "u16", U16);
    integer_range_tests!(u32_tests, u32, "u32", U32);
    integer_range_tests!(i8_tests, i8, "i8", I8);
    integer_range_tests!(i16_tests, i16, "i16", I16);
    integer_range_tests!(i32_tests, i32, "i32", I32);

    #[test]
    fn test_u64_range() {
        for value in [u64::MIN, 1, 1 << 53, u64::MAX - 2047] {
            assert_number_ok("u64", value as f64, KvsValue::U64(value));
        }
        assert_number_invalid("u64", -1.0);
        // Next `f64` above `u64::MAX as f64` (2^64).
        assert_number_invalid("u64", pow2(64) + pow2(12));
        assert_number_invalid("u64", 1.5);
        assert_number_invalid("u64", f64::INFINITY);
    }

    #[test]
    fn test_i64_range() {
        for value in [i64::MIN, -(1 << 53), 0, 1 << 53, i64::MAX - 1023] {
            assert_number_ok("i64", value as f64, KvsValue::I64(value));
        }
        assert_number_invalid("i64", -pow2(63) - pow2(11));
        // Next `f64` above `i64::MAX as f64` (2^63).
        assert_number_invalid("i64", pow2(63) + pow2(11));
        assert_number_invalid("i64", -1.5);
        assert_number_invalid("i64", f64::NEG_INFINITY);
    }

    #[test]
    fn test_u64_max() {
        // `u64::MAX` is written as 2^64.
        assert_number_ok("u64", pow2(64), KvsValue::U64(u64::MAX));
    }

    #[test]
    fn test_i64_max() {
        // `i64::MAX` is written as 2^63.
        assert_number_ok("i64", pow2(63), KvsValue::I64(i64::MAX));
    }

    #[test]
    fn test_f32_range() {
        for value in [
            f32::MIN,
            -1.5,
            -f32::MIN_POSITIVE,
            0.0,
            0.1,
            f32::MIN_POSITIVE,
            12.5,
            f32::MAX,
        ] {
            assert_number_ok("f32", value as f64, KvsValue::F32(value));
        }
    }

    #[test]
    fn test_f32_max_written_by_cpp() {
        // C++ writes `f32::MAX` in its shortest round-trip form, which parses slightly above `f32::MAX`.
        let parsed: f64 = "3.4028235e+38".parse().unwrap();
        assert!(parsed > f32::MAX as f64);
        assert_number_ok("f32", parsed, KvsValue::F32(f32::MAX));
        assert_number_ok("f32", -parsed, KvsValue::F32(f32::MIN));
    }

    #[test]
    fn test_f32_out_of_range() {
        // `f32::MAX + ulp / 2` is the smallest value rounding to infinity.
        let limit = f32::MAX as f64 + pow2(103);
        assert_number_ok("f32", limit - pow2(80), KvsValue::F32(f32::MAX));
        assert_number_invalid("f32", limit);
        assert_number_invalid("f32", -limit);
        assert_number_invalid("f32", f64::MAX);
        assert_number_invalid("f32", f64::NAN);
        assert_number_invalid("f32", f64::INFINITY);
    }

    #[test]
    fn test_f32_invalid_type() {
        let jv = tagged("f32", JsonValue::String("1.5".to_string()));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_f64_range() {
        for value in [f64::MIN, -432.1, 0.0, f64::MIN_POSITIVE, 0.1, f64::MAX] {
            assert_number_ok("f64", value, KvsValue::F64(value));
        }
        assert_number_invalid("f64", f64::NAN);
        assert_number_invalid("f64", f64::INFINITY);
    }

    #[test]
    fn test_i32_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::I32(-123));
    }

    #[test]
    fn test_i32_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::String("-123.0".to_string())),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_u32_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("u32".to_string())),
            ("v".to_string(), JsonValue::Number(123.0)),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::U32(123));
    }

    #[test]
    fn test_u32_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("u32".to_string())),
            ("v".to_string(), JsonValue::String("123.0".to_string())),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_i64_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i64".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::I64(-123));
    }

    #[test]
    fn test_i64_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i64".to_string())),
            ("v".to_string(), JsonValue::String("-123.0".to_string())),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_u64_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("u64".to_string())),
            ("v".to_string(), JsonValue::Number(123.0)),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::U64(123));
    }

    #[test]
    fn test_u64_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("u64".to_string())),
            ("v".to_string(), JsonValue::String("123.0".to_string())),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_f64_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("f64".to_string())),
            ("v".to_string(), JsonValue::Number(-432.1)),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::F64(-432.1));
    }

    #[test]
    fn test_f64_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("f64".to_string())),
            ("v".to_string(), JsonValue::String("-432.1".to_string())),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_bool_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("bool".to_string())),
            ("v".to_string(), JsonValue::Boolean(true)),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::Boolean(true));
    }

    #[test]
    fn test_bool_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("bool".to_string())),
            ("v".to_string(), JsonValue::String("true".to_string())),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_string_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("str".to_string())),
            ("v".to_string(), JsonValue::String("example".to_string())),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::String("example".to_string()));
    }

    #[test]
    fn test_string_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("str".to_string())),
            ("v".to_string(), JsonValue::Number(123.4)),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_null_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("null".to_string())),
            ("v".to_string(), JsonValue::Null),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::Null);
    }

    #[test]
    fn test_null_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("null".to_string())),
            ("v".to_string(), JsonValue::Number(123.4)),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
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
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::Array(vec![KvsValue::I32(-123), KvsValue::F64(555.5)]));
    }

    #[test]
    fn test_array_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("arr".to_string())),
            ("v".to_string(), JsonValue::String("example".to_string())),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
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
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(
            kv,
            KvsValue::Object(KvsMap::from([
                ("entry1".to_string(), KvsValue::try_from(entry1).unwrap()),
                ("entry2".to_string(), KvsValue::try_from(entry2).unwrap())
            ]))
        );
    }

    #[test]
    fn test_object_with_t_v_keys() {
        // "t" and "v" are valid user keys inside a type-tagged object.
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("obj".to_string())),
            (
                "v".to_string(),
                JsonValue::Object(HashMap::from([
                    (
                        "t".to_string(),
                        JsonValue::from(HashMap::from([
                            ("t".to_string(), JsonValue::String("str".to_string())),
                            ("v".to_string(), JsonValue::String("obj".to_string())),
                        ])),
                    ),
                    (
                        "v".to_string(),
                        JsonValue::from(HashMap::from([
                            ("t".to_string(), JsonValue::String("i32".to_string())),
                            ("v".to_string(), JsonValue::Number(5.0)),
                        ])),
                    ),
                ])),
            ),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(
            kv,
            KvsValue::Object(KvsMap::from([
                ("t".to_string(), KvsValue::String("obj".to_string())),
                ("v".to_string(), KvsValue::I32(5)),
            ]))
        );
    }

    #[test]
    fn test_object_invalid_type() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("obj".to_string())),
            ("v".to_string(), JsonValue::String("example".to_string())),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_array_empty_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("arr".to_string())),
            ("v".to_string(), JsonValue::Array(vec![])),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::Array(vec![]));
    }

    #[test]
    fn test_array_invalid_element() {
        let entry1 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::Number(-123.0)),
        ]));
        let entry2 = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::String("example".to_string())),
        ]));
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("arr".to_string())),
            ("v".to_string(), JsonValue::Array(vec![entry1, entry2])),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_array_untagged_element() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("arr".to_string())),
            ("v".to_string(), JsonValue::Array(vec![JsonValue::Number(123.0)])),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::ConversionFailed));
    }

    #[test]
    fn test_object_empty_ok() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("obj".to_string())),
            ("v".to_string(), JsonValue::Object(HashMap::new())),
        ]));
        let kv = KvsValue::try_from(jv).unwrap();
        assert_eq!(kv, KvsValue::Object(KvsMap::new()));
    }

    #[test]
    fn test_object_invalid_entry() {
        let entry = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("bool".to_string())),
            ("v".to_string(), JsonValue::Number(1.0)),
        ]));
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("obj".to_string())),
            (
                "v".to_string(),
                JsonValue::Object(HashMap::from([("entry".to_string(), entry)])),
            ),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_object_untagged_entry() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("obj".to_string())),
            (
                "v".to_string(),
                JsonValue::Object(HashMap::from([(
                    "entry".to_string(),
                    JsonValue::String("example".to_string()),
                )])),
            ),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::ConversionFailed));
    }

    #[test]
    fn test_unknown_type_tag() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("xyz".to_string())),
            ("v".to_string(), JsonValue::Number(1.0)),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_type_tag_not_string() {
        let jv = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::Number(5.0)),
            ("v".to_string(), JsonValue::Number(1.0)),
        ]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::ConversionFailed));
    }

    #[test]
    fn test_missing_type_tag() {
        let jv = JsonValue::from(HashMap::from([("v".to_string(), JsonValue::Number(1.0))]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::ConversionFailed));
    }

    #[test]
    fn test_missing_value() {
        let jv = JsonValue::from(HashMap::from([("t".to_string(), JsonValue::String("i32".to_string()))]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::ConversionFailed));
    }

    #[test]
    fn test_untagged_object() {
        // Plain key-value maps are no longer accepted, values must be type-tagged.
        let entry = JsonValue::from(HashMap::from([
            ("t".to_string(), JsonValue::String("i32".to_string())),
            ("v".to_string(), JsonValue::Number(1.0)),
        ]));
        let jv = JsonValue::from(HashMap::from([("k1".to_string(), entry)]));
        assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::ConversionFailed));
    }

    #[test]
    fn test_non_json_value_object() {
        let values = [
            JsonValue::Number(123.0),
            JsonValue::String("example".to_string()),
            JsonValue::Boolean(true),
            JsonValue::Null,
            JsonValue::Array(vec![JsonValue::Number(123.0)]),
        ];
        for jv in values {
            assert!(KvsValue::try_from(jv).is_err_and(|e| e == ErrorCode::ConversionFailed));
        }
    }
}

#[cfg(test)]
mod kvs_value_to_json_value_conversion_tests {
    use crate::kvs_value::{KvsMap, KvsValue};
    use std::collections::HashMap;
    use tinyjson::JsonValue;

    fn assert_tagged_number(kv: KvsValue, tag: &str, value: f64) {
        assert_eq!(
            JsonValue::try_from(kv).unwrap(),
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String(tag.to_string())),
                ("v".to_string(), JsonValue::Number(value)),
            ]))
        );
    }

    #[test]
    fn test_new_numeric_types_ok() {
        assert_tagged_number(KvsValue::U8(u8::MAX), "u8", 255.0);
        assert_tagged_number(KvsValue::U16(u16::MAX), "u16", 65535.0);
        assert_tagged_number(KvsValue::I8(i8::MIN), "i8", -128.0);
        assert_tagged_number(KvsValue::I16(i16::MIN), "i16", -32768.0);
        assert_tagged_number(KvsValue::F32(12.5), "f32", 12.5);
        assert_tagged_number(KvsValue::F32(f32::MAX), "f32", f32::MAX as f64);
    }

    #[test]
    fn test_i32_ok() {
        let kv = KvsValue::I32(-123);
        let jv = JsonValue::try_from(kv).unwrap();

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
        let jv = JsonValue::try_from(kv).unwrap();

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
        let jv = JsonValue::try_from(kv).unwrap();

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
        let jv = JsonValue::try_from(kv).unwrap();

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
        let jv = JsonValue::try_from(kv).unwrap();

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
        let jv = JsonValue::try_from(kv).unwrap();

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
        let jv = JsonValue::try_from(kv).unwrap();

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
        let jv = JsonValue::try_from(kv).unwrap();

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
        let jv = JsonValue::try_from(kv).unwrap();

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
            ("entry1".to_string(), KvsValue::try_from(entry1.clone()).unwrap()),
            ("entry2".to_string(), KvsValue::try_from(entry2.clone()).unwrap()),
        ]));
        let jv = JsonValue::try_from(kv).unwrap();

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

    #[test]
    fn test_array_empty_ok() {
        let kv = KvsValue::Array(vec![]);
        let jv = JsonValue::try_from(kv).unwrap();

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("arr".to_string())),
                ("v".to_string(), JsonValue::Array(vec![])),
            ]))
        );
    }

    #[test]
    fn test_object_empty_ok() {
        let kv = KvsValue::Object(KvsMap::new());
        let jv = JsonValue::try_from(kv).unwrap();

        assert_eq!(
            jv,
            JsonValue::Object(HashMap::from([
                ("t".to_string(), JsonValue::String("obj".to_string())),
                ("v".to_string(), JsonValue::Object(HashMap::new())),
            ]))
        );
    }

    #[test]
    fn test_round_trip() {
        let nested = KvsMap::from([
            ("t".to_string(), KvsValue::String("obj".to_string())),
            ("v".to_string(), KvsValue::I32(5)),
        ]);
        let kv = KvsValue::Object(KvsMap::from([
            ("i32".to_string(), KvsValue::I32(-123)),
            ("u32".to_string(), KvsValue::U32(123)),
            ("i64".to_string(), KvsValue::I64(-456)),
            ("u64".to_string(), KvsValue::U64(456)),
            ("f64".to_string(), KvsValue::F64(-432.1)),
            ("bool".to_string(), KvsValue::Boolean(true)),
            ("str".to_string(), KvsValue::String("example".to_string())),
            ("null".to_string(), KvsValue::Null),
            (
                "arr".to_string(),
                KvsValue::Array(vec![KvsValue::I32(1), KvsValue::Array(vec![KvsValue::Null])]),
            ),
            ("obj".to_string(), KvsValue::Object(nested)),
        ]));

        let jv = JsonValue::try_from(kv.clone()).unwrap();
        assert_eq!(KvsValue::try_from(jv).unwrap(), kv);
    }
}

#[cfg(test)]
mod json_text_round_trip_tests {
    use crate::kvs_value::KvsValue;
    use tinyjson::JsonValue;

    fn round_trip(kv: KvsValue) -> KvsValue {
        let text = JsonValue::try_from(kv).unwrap().stringify().unwrap();
        let parsed: JsonValue = text.parse().unwrap();
        KvsValue::try_from(parsed).unwrap_or_else(|e| panic!("{text}: {e:?}"))
    }

    macro_rules! round_trip_test {
        ($name:ident, $t:ty, $variant:ident, [$($value:expr),*]) => {
            #[test]
            fn $name() {
                let values: &[$t] = &[$($value),*];
                for &value in values {
                    assert_eq!(round_trip(KvsValue::$variant(value)), KvsValue::$variant(value));
                }
            }
        };
    }

    round_trip_test!(test_u8, u8, U8, [u8::MIN, 123, u8::MAX]);
    round_trip_test!(test_u16, u16, U16, [u16::MIN, 12345, u16::MAX]);
    round_trip_test!(test_u32, u32, U32, [u32::MIN, 1234567, u32::MAX]);
    round_trip_test!(test_u64, u64, U64, [u64::MIN, 123456789, 1 << 53]);
    round_trip_test!(test_i8, i8, I8, [i8::MIN, -123, 0, i8::MAX]);
    round_trip_test!(test_i16, i16, I16, [i16::MIN, -12345, 0, i16::MAX]);
    round_trip_test!(test_i32, i32, I32, [i32::MIN, -1234567, 0, i32::MAX]);
    round_trip_test!(test_i64, i64, I64, [i64::MIN, -123456789, 0, 1 << 53]);
    round_trip_test!(
        test_f32,
        f32,
        F32,
        [f32::MIN, -12.5, 0.0, 0.1, f32::MIN_POSITIVE, f32::MAX]
    );
    round_trip_test!(
        test_f64,
        f64,
        F64,
        [f64::MIN, -5432.1, 0.0, 0.1, f64::MIN_POSITIVE, f64::MAX]
    );

    #[test]
    fn test_u64_max() {
        assert_eq!(round_trip(KvsValue::U64(u64::MAX)), KvsValue::U64(u64::MAX));
    }

    #[test]
    fn test_i64_max() {
        assert_eq!(round_trip(KvsValue::I64(i64::MAX)), KvsValue::I64(i64::MAX));
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
    use crate::json_backend::{JsonBackend, JsonBackendBuilder};
    use crate::kvs_api::{InstanceId, SnapshotId};
    use crate::prelude::KvsBackend;
    use crate::test_utils::tempdir;
    use std::path::{Path, PathBuf};

    /// Get the working directory of a backend from the parent of its KVS file path.
    fn working_dir_of(backend: &JsonBackend) -> Option<PathBuf> {
        backend
            .kvs_file_path(InstanceId(0), SnapshotId(0))
            .parent()
            .map(Path::to_path_buf)
    }

    #[test]
    fn test_new_ok() {
        let backend = JsonBackendBuilder::new().build();
        assert_eq!(working_dir_of(&backend), Some(PathBuf::new()));
        assert_eq!(backend.snapshot_max_count(), 3);
    }

    #[test]
    fn test_default_ok() {
        let backend = JsonBackendBuilder::default().build();
        assert_eq!(working_dir_of(&backend), Some(PathBuf::new()));
        assert_eq!(backend.snapshot_max_count(), 3);
    }

    #[test]
    fn test_working_dir_ok() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        assert_eq!(working_dir_of(&backend), Some(dir_path));
        assert_eq!(backend.snapshot_max_count(), 3);
    }

    #[test]
    fn test_snapshot_max_count_ok() {
        let backend = JsonBackendBuilder::new().snapshot_max_count(10).build();
        assert_eq!(working_dir_of(&backend), Some(PathBuf::new()));
        assert_eq!(backend.snapshot_max_count(), 10);
    }

    #[test]
    fn test_chained_ok() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new()
            .working_dir(dir_path.clone())
            .snapshot_max_count(10)
            .build();
        assert_eq!(working_dir_of(&backend), Some(dir_path.clone()));
        assert_eq!(backend.snapshot_max_count(), 10);

        // Setter order must not matter.
        let reversed = JsonBackendBuilder::new()
            .snapshot_max_count(10)
            .working_dir(dir_path)
            .build();
        assert_eq!(backend, reversed);
    }
}

#[cfg(test)]
mod json_backend_tests {
    use crate::error_code::ErrorCode;
    use crate::json_backend::{JsonBackend, JsonBackendBuilder};
    use crate::kvs_api::{InstanceId, SnapshotId, MAX_KEY_LENGTH_BYTES};
    use crate::kvs_backend::KvsBackend;
    use crate::kvs_value::{KvsMap, KvsValue};
    use crate::test_utils::tempdir;
    use std::os::unix::fs::PermissionsExt;
    use std::path::{Path, PathBuf};

    /// Expected permissions of KVS JSON and hash files: 0640.
    const EXPECTED_FILE_MODE: u32 = 0o640;

    const INSTANCE_ID: InstanceId = InstanceId(1);
    const SNAPSHOT_ID: SnapshotId = SnapshotId(0);

    fn file_mode(path: &Path) -> u32 {
        std::fs::metadata(path).unwrap().permissions().mode() & 0o777
    }

    fn temp_file_path(path: &Path) -> PathBuf {
        PathBuf::from(format!("{}.tmp", path.display()))
    }

    fn single_snapshot_backend(working_dir: &Path) -> JsonBackend {
        // One snapshot means `flush` always overwrites snapshot 0 and never rotates.
        JsonBackendBuilder::new()
            .working_dir(working_dir.to_path_buf())
            .snapshot_max_count(1)
            .build()
    }

    fn create_kvs_files(backend: &JsonBackend) -> (PathBuf, PathBuf) {
        let kvs_map = KvsMap::from([
            ("k1".to_string(), KvsValue::from("v1")),
            ("k2".to_string(), KvsValue::from(true)),
            ("k3".to_string(), KvsValue::from(123.4)),
        ]);
        backend.flush(INSTANCE_ID, &kvs_map).unwrap();
        (
            backend.kvs_file_path(INSTANCE_ID, SNAPSHOT_ID),
            backend.hash_file_path(INSTANCE_ID, SNAPSHOT_ID),
        )
    }

    /// Write `contents` as the KVS file with its valid hash.
    fn write_kvs_files(backend: &JsonBackend, contents: &str) {
        let hash = adler32::RollingAdler32::from_buffer(contents.as_bytes()).hash();
        std::fs::write(backend.kvs_file_path(INSTANCE_ID, SNAPSHOT_ID), contents).unwrap();
        std::fs::write(backend.hash_file_path(INSTANCE_ID, SNAPSHOT_ID), hash.to_be_bytes()).unwrap();
    }

    #[test]
    fn test_load_ok() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        create_kvs_files(&backend);

        let kvs_map = backend.load_kvs(INSTANCE_ID, SNAPSHOT_ID).unwrap();
        assert_eq!(kvs_map.len(), 3);
    }

    #[test]
    fn test_load_kvs_not_found() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        let (kvs_path, _hash_path) = create_kvs_files(&backend);
        std::fs::remove_file(&kvs_path).unwrap();

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::FileNotFound));
    }

    // Public paths always end in `.json` and `.hash`, so `check_path_extensions` can't fail through `load_kvs` or `flush`.
    // The extension tests call `load` and `save` directly.
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
        let backend = single_snapshot_backend(dir.path());
        let (_kvs_path, hash_path) = create_kvs_files(&backend);
        std::fs::remove_file(&hash_path).unwrap();

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::FileNotFound));
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
    fn test_load_t_v_keys() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());

        // "t" and "v" are valid user keys inside the type-tagged root object.
        let contents = r#"{"t":"obj","v":{"t":{"t":"str","v":"obj"},"v":{"t":"i32","v":5}}}"#;
        write_kvs_files(&backend, contents);

        let kvs_map = backend.load_kvs(INSTANCE_ID, SNAPSHOT_ID).unwrap();
        assert_eq!(
            kvs_map,
            KvsMap::from([
                ("t".to_string(), KvsValue::String("obj".to_string())),
                ("v".to_string(), KvsValue::I32(5)),
            ])
        );
    }

    #[test]
    fn test_load_malformed_json() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        write_kvs_files(&backend, "{\"malformed_json\"}");

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::JsonParserError));
    }

    #[test]
    fn test_load_invalid_data() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        write_kvs_files(&backend, "[123.4, 567.8]");

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::ConversionFailed));
    }

    #[test]
    fn test_load_empty_ok() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());

        let contents = r#"{"t":"obj","v":{}}"#;
        write_kvs_files(&backend, contents);

        let kvs_map = backend.load_kvs(INSTANCE_ID, SNAPSHOT_ID).unwrap();
        assert_eq!(kvs_map, KvsMap::new());
    }

    #[test]
    fn test_load_untagged_root() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());

        // Root object without type tag is no longer accepted.
        let contents = r#"{"k1":{"t":"str","v":"v1"}}"#;
        write_kvs_files(&backend, contents);

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::ConversionFailed));
    }

    #[test]
    fn test_load_tagged_non_object_root() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());

        let contents = r#"{"t":"i32","v":5}"#;
        write_kvs_files(&backend, contents);

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::JsonParserError));
    }

    #[test]
    fn test_load_invalid_key() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());

        // Key that is not valid found while loading means the file is invalid/corrupted.
        // Keys are placed in JSON as-is - control characters must be escaped.
        let invalid_keys = vec![
            String::new(),
            "k".repeat(MAX_KEY_LENGTH_BYTES + 1),
            "\\u001f".to_string(),
            "\\u007f".to_string(),
            "tab\\tkey".to_string(),
            "ą".to_string(),
            "\\u0080".to_string(),
        ];
        for invalid_key in invalid_keys {
            let contents = format!(
                "{{\"t\": \"obj\", \"v\": {{\"valid_key\": {{\"t\": \"i32\", \"v\": 42}}, \"{invalid_key}\": {{\"t\": \"i32\", \"v\": 7}}}}}}"
            );
            write_kvs_files(&backend, &contents);

            assert!(
                backend
                    .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
                    .is_err_and(|e| e == ErrorCode::InvalidKey),
                "key: {invalid_key:?}"
            );
        }
    }

    #[test]
    fn test_load_invalid_value_type() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());

        let contents = r#"{"t":"obj","v":{"k1":{"t":"i32","v":"x"}}}"#;
        write_kvs_files(&backend, contents);

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::InvalidValueType));
    }

    #[test]
    fn test_load_invalid_hash_content() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        let (_kvs_path, hash_path) = create_kvs_files(&backend);
        std::fs::write(hash_path, vec![0x12, 0x34, 0x56, 0x78]).unwrap();

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::ValidationFailed));
    }

    #[test]
    fn test_load_invalid_hash_len() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        let (_kvs_path, hash_path) = create_kvs_files(&backend);
        std::fs::write(hash_path, vec![0x12, 0x34, 0x56]).unwrap();

        assert!(backend
            .load_kvs(INSTANCE_ID, SNAPSHOT_ID)
            .is_err_and(|e| e == ErrorCode::ValidationFailed));
    }

    #[test]
    fn test_save_ok() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());

        let kvs_map = KvsMap::from([
            ("k1".to_string(), KvsValue::from("v1")),
            ("k2".to_string(), KvsValue::from(true)),
            ("k3".to_string(), KvsValue::from(123.4)),
        ]);
        backend.flush(INSTANCE_ID, &kvs_map).unwrap();

        assert!(backend.kvs_file_path(INSTANCE_ID, SNAPSHOT_ID).exists());
        assert!(backend.hash_file_path(INSTANCE_ID, SNAPSHOT_ID).exists());
    }

    #[cfg_attr(miri, ignore)]
    #[test]
    fn test_save_sets_file_permissions() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        let (kvs_path, hash_path) = create_kvs_files(&backend);

        assert_eq!(file_mode(&kvs_path), EXPECTED_FILE_MODE);
        assert_eq!(file_mode(&hash_path), EXPECTED_FILE_MODE);
    }

    #[cfg_attr(miri, ignore)]
    #[test]
    fn test_save_overrides_existing_file_permissions() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        let (kvs_path, hash_path) = create_kvs_files(&backend);

        // Widen permissions of existing files.
        let all_perms = 0o777;
        std::fs::set_permissions(&kvs_path, std::fs::Permissions::from_mode(all_perms)).unwrap();
        std::fs::set_permissions(&hash_path, std::fs::Permissions::from_mode(all_perms)).unwrap();
        assert_eq!(file_mode(&kvs_path), all_perms);
        assert_eq!(file_mode(&hash_path), all_perms);

        // Save again.
        let kvs_map = KvsMap::from([("k1".to_string(), KvsValue::from("v1"))]);
        backend.flush(INSTANCE_ID, &kvs_map).unwrap();

        assert_eq!(file_mode(&kvs_path), EXPECTED_FILE_MODE);
        assert_eq!(file_mode(&hash_path), EXPECTED_FILE_MODE);
        // The same files were rewritten, not rotated to snapshot 1.
        assert_eq!(backend.snapshot_count(INSTANCE_ID), 1);
        assert!(!backend.kvs_file_path(INSTANCE_ID, SnapshotId(1)).exists());
        assert!(!backend.hash_file_path(INSTANCE_ID, SnapshotId(1)).exists());
    }

    #[cfg_attr(miri, ignore)]
    #[test]
    fn test_save_replaces_stale_temp_file() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        let kvs_path = backend.kvs_file_path(INSTANCE_ID, SNAPSHOT_ID);

        // Create temporary file left behind by an interrupted write.
        let kvs_tmp_path = temp_file_path(&kvs_path);
        std::fs::write(&kvs_tmp_path, "stale").unwrap();
        std::fs::set_permissions(&kvs_tmp_path, std::fs::Permissions::from_mode(0o777)).unwrap();

        let kvs_map = KvsMap::from([("k1".to_string(), KvsValue::from("v1"))]);
        backend.flush(INSTANCE_ID, &kvs_map).unwrap();

        assert!(!kvs_tmp_path.exists());
        assert_eq!(backend.load_kvs(INSTANCE_ID, SNAPSHOT_ID).unwrap(), kvs_map);
        assert_eq!(file_mode(&kvs_path), EXPECTED_FILE_MODE);
    }

    #[test]
    fn test_save_leaves_no_temp_files() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        let (kvs_path, hash_path) = create_kvs_files(&backend);

        assert!(!temp_file_path(&kvs_path).exists());
        assert!(!temp_file_path(&hash_path).exists());
    }

    #[test]
    fn test_save_staging_failure_keeps_previous_file() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());
        let (kvs_path, hash_path) = create_kvs_files(&backend);
        let previous_kvs = std::fs::read(&kvs_path).unwrap();
        let previous_hash = std::fs::read(&hash_path).unwrap();

        // Temporary file path is occupied by a non-empty directory - it cannot be removed or created.
        let kvs_tmp_path = temp_file_path(&kvs_path);
        std::fs::create_dir(&kvs_tmp_path).unwrap();
        std::fs::write(kvs_tmp_path.join("file"), "data").unwrap();

        let kvs_map = KvsMap::from([("k1".to_string(), KvsValue::from("v1"))]);
        assert!(backend
            .flush(INSTANCE_ID, &kvs_map)
            .is_err_and(|e| e == ErrorCode::PhysicalStorageFailure));

        assert_eq!(std::fs::read(&kvs_path).unwrap(), previous_kvs);
        assert_eq!(std::fs::read(&hash_path).unwrap(), previous_hash);
        assert!(!temp_file_path(&hash_path).exists());
    }

    #[cfg_attr(miri, ignore)]
    #[test]
    fn test_save_read_only_dir_physical_storage_failure() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = single_snapshot_backend(&dir_path);
        let (kvs_path, hash_path) = create_kvs_files(&backend);
        let previous_kvs = std::fs::read(&kvs_path).unwrap();
        let previous_hash = std::fs::read(&hash_path).unwrap();

        std::fs::set_permissions(&dir_path, std::fs::Permissions::from_mode(0o500)).unwrap();
        // Directory mode is ignored by privileged users (root, `CAP_DAC_OVERRIDE`) - test is not applicable.
        if std::fs::File::create(dir_path.join("probe")).is_ok() {
            std::fs::set_permissions(&dir_path, std::fs::Permissions::from_mode(0o700)).unwrap();
            return;
        }

        let kvs_map = KvsMap::from([("k1".to_string(), KvsValue::from("v1"))]);
        let result = backend.flush(INSTANCE_ID, &kvs_map);
        std::fs::set_permissions(&dir_path, std::fs::Permissions::from_mode(0o700)).unwrap();

        assert!(result.is_err_and(|e| e == ErrorCode::PhysicalStorageFailure));
        assert_eq!(std::fs::read(&kvs_path).unwrap(), previous_kvs);
        assert_eq!(std::fs::read(&hash_path).unwrap(), previous_hash);
    }

    #[test]
    fn test_save_overwrite_round_trip() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(dir.path());

        let first = KvsMap::from([("k1".to_string(), KvsValue::from("a much longer first value"))]);
        backend.flush(INSTANCE_ID, &first).unwrap();
        let second = KvsMap::from([("k1".to_string(), KvsValue::from("v2"))]);
        backend.flush(INSTANCE_ID, &second).unwrap();

        let loaded = backend.load_kvs(INSTANCE_ID, SNAPSHOT_ID).unwrap();
        assert_eq!(loaded, second);
    }

    #[test]
    fn test_save_missing_dir() {
        let dir = tempdir().unwrap();
        let backend = single_snapshot_backend(&dir.path().join("missing"));

        let result = backend.flush(INSTANCE_ID, &KvsMap::new());
        assert!(result.is_err_and(|e| e == ErrorCode::PhysicalStorageFailure));
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
        let backend = single_snapshot_backend(dir.path());

        let kvs_map = KvsMap::from([("inf".to_string(), KvsValue::from(f64::INFINITY))]);

        assert!(backend
            .flush(INSTANCE_ID, &kvs_map)
            .is_err_and(|e| e == ErrorCode::JsonGeneratorError));
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
    use crate::test_utils::tempdir;
    use std::fs;
    use std::os::unix::fs::PermissionsExt;
    use std::path::Path;

    /// Expected permissions of KVS JSON and hash files: 0640.
    const EXPECTED_FILE_MODE: u32 = 0o640;

    fn file_mode(path: &Path) -> u32 {
        fs::metadata(path).unwrap().permissions().mode() & 0o777
    }

    fn create_kvs_files(backend: &JsonBackend, instance_id: InstanceId, snapshot_id: SnapshotId) {
        assert!(snapshot_id.0 < backend.snapshot_max_count());
        let kvs_map = KvsMap::from([
            ("k1".to_string(), KvsValue::from("v1")),
            ("k2".to_string(), KvsValue::from(true)),
            ("k3".to_string(), KvsValue::from(123.4)),
        ]);
        // Each `flush` rotates the existing snapshots up by one.
        for _ in 0..=snapshot_id.0 {
            backend.flush(instance_id, &kvs_map).unwrap();
        }
        // Remove the lower snapshots, so only the requested snapshot can be loaded.
        for idx in 0..snapshot_id.0 {
            fs::remove_file(backend.kvs_file_path(instance_id, SnapshotId(idx))).unwrap();
            fs::remove_file(backend.hash_file_path(instance_id, SnapshotId(idx))).unwrap();
        }
    }

    // There is no public writer for the defaults file, so this helper calls `save` directly.
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

    #[cfg_attr(miri, ignore)]
    #[test]
    fn test_flush_sets_file_permissions() {
        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();
        let backend = JsonBackendBuilder::new().working_dir(dir_path).build();
        let instance_id = InstanceId(1);

        // Flush.
        let kvs_map = KvsMap::from([("key".to_string(), KvsValue::from("value"))]);
        backend.flush(instance_id, &kvs_map).unwrap();

        // Check permissions.
        let snapshot_id = SnapshotId(0);
        let kvs_path = backend.kvs_file_path(instance_id, snapshot_id);
        let hash_path = backend.hash_file_path(instance_id, snapshot_id);
        assert_eq!(file_mode(&kvs_path), EXPECTED_FILE_MODE);
        assert_eq!(file_mode(&hash_path), EXPECTED_FILE_MODE);
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

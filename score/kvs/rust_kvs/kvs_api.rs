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
use crate::kvs_value::KvsValue;
use crate::log::ScoreDebug;

/// Maximum key length in bytes.
pub const MAX_KEY_LENGTH_BYTES: usize = 32;

/// Maximum key length in bytes.
#[deprecated(note = "Use `MAX_KEY_LENGTH_BYTES` instead.")]
pub const KVS_MAX_KEY_LENGTH_BYTES: usize = MAX_KEY_LENGTH_BYTES;

/// Validate that:
/// - Key length is in valid range - (0; `MAX_KEY_LENGTH_BYTES`>.
/// - Key consists solely of printable ASCII characters (0x20 - 0x7E).
pub(crate) fn is_key_valid(key: &str) -> bool {
    // comp_req__kvs__key_length: disallow empty keys and keys too long.
    if key.is_empty() || key.len() > MAX_KEY_LENGTH_BYTES {
        return false;
    }

    // comp_req__kvs__key_naming: check for allowed characters.
    // comp_req__kvs__key_encoding: allowed characters enforce encoding.
    key.bytes().all(|b| (0x20..=0x7E).contains(&b))
}

/// Instance ID
#[derive(Clone, Copy, Debug, PartialEq, Eq, ScoreDebug)]
pub struct InstanceId(pub usize);

impl core::fmt::Display for InstanceId {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl From<InstanceId> for usize {
    fn from(value: InstanceId) -> Self {
        value.0
    }
}

/// Snapshot ID
#[derive(Clone, Copy, Debug, PartialEq, Eq, ScoreDebug)]
pub struct SnapshotId(pub usize);

impl core::fmt::Display for SnapshotId {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl From<SnapshotId> for usize {
    fn from(value: SnapshotId) -> Self {
        value.0
    }
}

/// Defaults handling mode.
#[derive(Clone, Copy, Debug, PartialEq, Eq, ScoreDebug)]
#[repr(u8)]
pub enum KvsDefaults {
    /// Defaults must be loaded.
    Required,

    /// Defaults are loaded if available.
    Optional,

    /// Defaults are not loaded.
    Ignored,
}

/// KVS load mode.
#[derive(Clone, Copy, Debug, PartialEq, Eq, ScoreDebug)]
#[repr(u8)]
pub enum KvsLoad {
    /// KVS must be loaded.
    Required,

    /// KVS is loaded if available.
    Optional,

    /// KVS is not loaded.
    Ignored,
}

pub trait KvsApi {
    fn reset(&self) -> Result<(), ErrorCode>;
    fn set_value_to_default(&self, key: &str) -> Result<(), ErrorCode>;
    fn get_all_keys(&self) -> Result<Vec<String>, ErrorCode>;
    fn key_exists(&self, key: &str) -> Result<bool, ErrorCode>;
    fn get_value(&self, key: &str) -> Result<KvsValue, ErrorCode>;
    fn get_value_as<T>(&self, key: &str) -> Result<T, ErrorCode>
    where
        for<'a> T: TryFrom<&'a KvsValue>,
        for<'a> <T as TryFrom<&'a KvsValue>>::Error: ScoreDebug;
    fn get_default_value(&self, key: &str) -> Result<KvsValue, ErrorCode>;
    fn is_value_default(&self, key: &str) -> Result<bool, ErrorCode>;
    fn set_value<S: Into<String>, J: Into<KvsValue>>(&self, key: S, value: J) -> Result<(), ErrorCode>;
    fn remove_key(&self, key: &str) -> Result<(), ErrorCode>;
    fn flush(&self) -> Result<(), ErrorCode>;
    fn snapshot_count(&self) -> usize;
    fn snapshot_max_count(&self) -> usize;
    fn snapshot_restore(&self, snapshot_id: SnapshotId) -> Result<(), ErrorCode>;
}

#[cfg(test)]
mod kvs_api_tests {
    use crate::kvs_api::{InstanceId, SnapshotId};

    #[test]
    fn test_instance_id_to_string() {
        let id = InstanceId(123);
        assert_eq!(id.to_string(), "123");
    }

    #[test]
    fn test_instance_id_to_usize() {
        let id = InstanceId(999);
        assert_eq!(usize::from(id), 999);
    }

    #[test]
    fn test_snapshot_id_fmt() {
        let id = SnapshotId(4321);
        assert_eq!(id.to_string(), "4321");
    }

    #[test]
    fn test_snapshot_id_to_usize() {
        let id = SnapshotId(0);
        assert_eq!(usize::from(id), 0);
    }
}

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
use crate::json_backend::JsonBackendBuilder;
use crate::kvs::{Kvs, KvsParameters};
use crate::kvs_api::{InstanceId, KvsDefaults, KvsLoad, SnapshotId};
use crate::kvs_backend::KvsBackend;
use crate::kvs_value::KvsMap;
use crate::log::{debug, error, info, trace, ScoreDebug};
use std::sync::{Arc, LazyLock, Mutex, MutexGuard, PoisonError};

/// Maximum number of instances.
const KVS_MAX_INSTANCES: usize = 10;

/// KVS instance data.
/// Expected to be shared between instance pool and instances.
pub(crate) struct KvsData {
    /// Storage data.
    pub(crate) kvs_map: KvsMap,

    /// Optional default values.
    pub(crate) defaults_map: KvsMap,
}

impl From<PoisonError<MutexGuard<'_, KvsData>>> for ErrorCode {
    fn from(cause: PoisonError<MutexGuard<'_, KvsData>>) -> Self {
        error!("KVS data lock failed: {:?}", cause);
        ErrorCode::MutexLockFailed
    }
}

/// KVS instance inner representation.
pub(crate) struct KvsInner {
    /// KVS instance parameters.
    pub(crate) parameters: Arc<KvsParameters>,

    /// KVS instance data.
    pub(crate) data: Arc<Mutex<KvsData>>,
}

static KVS_POOL: LazyLock<Mutex<[Option<KvsInner>; KVS_MAX_INSTANCES]>> =
    LazyLock::new(|| Mutex::new([const { None }; KVS_MAX_INSTANCES]));

impl From<PoisonError<MutexGuard<'_, [Option<KvsInner>; KVS_MAX_INSTANCES]>>> for ErrorCode {
    fn from(cause: PoisonError<MutexGuard<'_, [Option<KvsInner>; KVS_MAX_INSTANCES]>>) -> Self {
        error!("KVS instance pool lock failed: {:?}", cause);
        ErrorCode::MutexLockFailed
    }
}

/// Key-value-storage builder.
#[derive(ScoreDebug)]
pub struct KvsBuilder {
    /// Instance ID.
    instance_id: InstanceId,

    /// Defaults handling mode.
    defaults: Option<KvsDefaults>,

    /// KVS load mode.
    kvs_load: Option<KvsLoad>,

    /// Backend.
    backend: Option<Box<dyn KvsBackend>>,
}

impl KvsBuilder {
    /// Create a builder to open the key-value-storage
    ///
    /// Only the instance ID must be set. All other settings are using default values until changed
    /// via the builder API.
    ///
    /// # Parameters
    ///   * `instance_id`: Instance ID
    ///
    /// # Return Values
    ///   * KvsBuilder instance
    pub fn new(instance_id: InstanceId) -> Self {
        Self {
            instance_id,
            defaults: None,
            kvs_load: None,
            backend: None,
        }
    }

    /// Return maximum number of allowed KVS instances.
    ///
    /// # Return Values
    ///   * Max number of KVS instances
    pub fn max_instances() -> usize {
        KVS_MAX_INSTANCES
    }

    /// Configure defaults handling mode.
    ///
    /// # Parameters
    ///   * `mode`: defaults handling mode (default: [`KvsDefaults::Optional`](KvsDefaults::Optional))
    ///
    /// # Return Values
    ///   * KvsBuilder instance
    pub fn defaults(mut self, mode: KvsDefaults) -> Self {
        trace!("'defaults' set to {:?}", mode);
        self.defaults = Some(mode);
        self
    }

    /// Configure KVS load mode.
    ///
    /// # Parameters
    ///   * `mode`: KVS load mode (default: [`KvsLoad::Optional`](KvsLoad::Optional))
    ///
    /// # Return Values
    ///   * KvsBuilder instance
    pub fn kvs_load(mut self, mode: KvsLoad) -> Self {
        trace!("'kvs_load' set to {:?}", mode);
        self.kvs_load = Some(mode);
        self
    }

    /// Set backend.
    /// Default backend is used if not set.
    ///
    /// # Parameters
    ///   * `backend`: KVS backend.
    ///
    /// # Return Values
    ///   * KvsBuilder instance
    pub fn backend(mut self, backend: Box<dyn KvsBackend>) -> Self {
        trace!("'backend' set to {:?}", backend);
        self.backend = Some(backend);
        self
    }

    /// Compare existing parameters with expected configuration.
    fn compare_parameters(&self, other: &KvsParameters) -> bool {
        // Compare instance ID.
        if self.instance_id != other.instance_id {
            error!("Instance ID mismatched");
            false
        }
        // Compare defaults handling mode.
        else if self.defaults.is_some_and(|v| v != other.defaults) {
            error!("Defaults handling mode mismatched");
            false
        }
        // Compare KVS load mode.
        else if self.kvs_load.is_some_and(|v| v != other.kvs_load) {
            error!("KVS load mode mismatched");
            false
        }
        // Compare backend.
        else if self.backend.as_ref().is_some_and(|v| !v.dyn_eq(other.backend.as_any())) {
            error!("Backend parameters mismatched");
            false
        }
        // Success.
        else {
            true
        }
    }

    /// Finalize the builder and open the key-value-storage
    ///
    /// Calls `Kvs::open` with the configured settings.
    ///
    /// # Features
    ///   * `FEAT_REQ__KVS__default_values`
    ///   * `FEAT_REQ__KVS__multiple_kvs`
    ///   * `FEAT_REQ__KVS__integrity_check`
    ///
    /// # Return Values
    ///   * Ok: KVS instance
    ///   * `ErrorCode::ValidationFailed`: KVS hash validation failed
    ///   * `ErrorCode::JsonParserError`: JSON parser error
    ///   * `ErrorCode::KvsFileReadError`: KVS file read error
    ///   * `ErrorCode::KvsHashFileReadError`: KVS hash file read error
    ///   * `ErrorCode::UnmappedError`: Generic error
    pub fn build(self) -> Result<Kvs, ErrorCode> {
        let instance_id = self.instance_id;
        let instance_id_index: usize = instance_id.into();

        debug!("Requested KVS instance with ID: {}", instance_id);

        // Check if instance already exists.
        {
            debug!("Checking for existing KVS instance in instance pool");
            let kvs_pool = KVS_POOL.lock()?;
            let kvs_inner_option = match kvs_pool.get(instance_id_index) {
                Some(kvs_pool_entry) => match kvs_pool_entry {
                    // If instance exists then parameters must match.
                    Some(kvs_inner) => {
                        if self.compare_parameters(&kvs_inner.parameters) {
                            debug!("Using KVS instance from instance pool");
                            Ok(Some(kvs_inner))
                        } else {
                            error!(
                                "Requested KVS instance parameters mismatch, provided: {:?}, available: {:?}",
                                self, kvs_inner.parameters
                            );
                            Err(ErrorCode::InstanceParametersMismatch)
                        }
                    },
                    // Instance not found - not an error, will initialize later.
                    None => {
                        debug!("KVS instance not found in instance pool");
                        Ok(None)
                    },
                },
                // Instance ID out of range.
                None => {
                    error!("Provided instance ID is out of range: {}", instance_id);
                    Err(ErrorCode::InvalidInstanceId)
                },
            }?;

            // Return existing instance if initialized.
            if let Some(kvs_inner) = kvs_inner_option {
                return Ok(Kvs::new(kvs_inner.data.clone(), kvs_inner.parameters.clone()));
            }
        }

        // Initialize KVS instance with provided parameters.
        let parameters = KvsParameters {
            instance_id,
            defaults: self.defaults.unwrap_or(KvsDefaults::Optional),
            kvs_load: self.kvs_load.unwrap_or(KvsLoad::Optional),
            backend: self.backend.unwrap_or(Box::new(JsonBackendBuilder::new().build())),
        };

        // Load defaults.
        debug!("Loading defaults");
        let defaults_map = match parameters.defaults {
            KvsDefaults::Ignored => KvsMap::new(),
            KvsDefaults::Optional => match parameters.backend.load_defaults(instance_id) {
                Ok(map) => map,
                Err(e) => match e {
                    ErrorCode::FileNotFound => KvsMap::new(),
                    _ => return Err(e),
                },
            },
            KvsDefaults::Required => parameters.backend.load_defaults(instance_id)?,
        };

        // Load KVS and hash files.
        debug!("Loading KVS data");
        let snapshot_id = SnapshotId(0);
        let kvs_map = match parameters.kvs_load {
            KvsLoad::Ignored => KvsMap::new(),
            KvsLoad::Optional => match parameters.backend.load_kvs(instance_id, snapshot_id) {
                Ok(map) => map,
                Err(e) => match e {
                    ErrorCode::FileNotFound => KvsMap::new(),
                    _ => return Err(e),
                },
            },
            KvsLoad::Required => parameters.backend.load_kvs(instance_id, snapshot_id)?,
        };

        // Shared object containing data.
        let data = Arc::new(Mutex::new(KvsData { kvs_map, defaults_map }));

        // Shared object containing parameters.
        let parameters = Arc::new(parameters);

        // Initialize entry in pool and return new KVS instance.
        {
            debug!("Initializing instance pool entry");
            let mut kvs_pool = KVS_POOL.lock()?;
            let kvs_pool_entry = match kvs_pool.get_mut(instance_id_index) {
                Some(entry) => entry,
                None => {
                    // Unlikely - this was checked previously.
                    error!("Provided instance ID is out of range: {}", instance_id);
                    return Err(ErrorCode::InvalidInstanceId);
                },
            };

            let _ = kvs_pool_entry.insert(KvsInner {
                parameters: parameters.clone(),
                data: data.clone(),
            });
        }

        info!("KVS instance initialized: {:?}", parameters.clone());
        Ok(Kvs::new(data, parameters))
    }
}

#[cfg(test)]
mod kvs_builder_tests {
    // Tests reuse JSON backend to ensure valid load/save behavior.
    use crate::error_code::ErrorCode;
    use crate::json_backend::{JsonBackend, JsonBackendBuilder};
    use crate::kvs_api::{InstanceId, KvsDefaults, KvsLoad, SnapshotId};
    use crate::kvs_builder::{KvsBuilder, KVS_MAX_INSTANCES, KVS_POOL};
    use crate::kvs_value::{KvsMap, KvsValue};
    use core::ops::DerefMut;
    use std::path::{Path, PathBuf};
    use std::sync::{LazyLock, Mutex, MutexGuard};
    use tempfile::tempdir;

    /// Serial test execution mutex.
    static SERIAL_TEST: LazyLock<Mutex<()>> = LazyLock::new(|| Mutex::new(()));

    /// Execute test serially with KVS pool uninitialized.
    fn lock_and_reset<'a>() -> MutexGuard<'a, ()> {
        // Tests in this group must be executed serially.
        let serial_lock: MutexGuard<'a, ()> = SERIAL_TEST.lock().unwrap();

        // Reset `KVS_POOL` state to uninitialized.
        // This is to mitigate `InstanceParametersMismatch` errors between tests.
        let mut pool = KVS_POOL.lock().unwrap();
        *pool.deref_mut() = [const { None }; KVS_MAX_INSTANCES];

        serial_lock
    }

    #[test]
    fn test_new_ok() {
        let _lock = lock_and_reset();

        // Check only if panic happens.
        let instance_id = InstanceId(0);
        let _ = KvsBuilder::new(instance_id);
    }

    #[test]
    fn test_max_instances() {
        assert_eq!(KvsBuilder::max_instances(), KVS_MAX_INSTANCES);
    }

    #[test]
    fn test_parameters_instance_id() {
        let _lock = lock_and_reset();

        let instance_id = InstanceId(1);
        let builder = KvsBuilder::new(instance_id);
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().instance_id, instance_id);
        // Check default values.
        assert_eq!(kvs.parameters().defaults, KvsDefaults::Optional);
        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Optional);
        assert!(kvs.parameters().backend.dyn_eq(&JsonBackendBuilder::new().build()));
    }

    #[test]
    fn test_parameters_defaults() {
        let _lock = lock_and_reset();

        let instance_id = InstanceId(1);
        let builder = KvsBuilder::new(instance_id).defaults(KvsDefaults::Ignored);
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().instance_id, instance_id);
        assert_eq!(kvs.parameters().defaults, KvsDefaults::Ignored);
        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Optional);
        assert!(kvs.parameters().backend.dyn_eq(&JsonBackendBuilder::new().build()));
    }

    #[test]
    fn test_parameters_kvs_load() {
        let _lock = lock_and_reset();

        let instance_id = InstanceId(1);
        let builder = KvsBuilder::new(instance_id).kvs_load(KvsLoad::Ignored);
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().instance_id, instance_id);
        assert_eq!(kvs.parameters().defaults, KvsDefaults::Optional);
        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Ignored);
        assert!(kvs.parameters().backend.dyn_eq(&JsonBackendBuilder::new().build()));
    }

    #[test]
    fn test_parameters_backend() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(5);
        let backend = JsonBackendBuilder::new()
            .working_dir(dir_path.clone())
            .snapshot_max_count(1234)
            .build();
        let builder = KvsBuilder::new(instance_id).backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().instance_id, instance_id);
        assert_eq!(kvs.parameters().defaults, KvsDefaults::Optional);
        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Optional);
        assert!(kvs.parameters().backend.dyn_eq(
            &JsonBackendBuilder::new()
                .working_dir(dir_path)
                .snapshot_max_count(1234)
                .build()
        ));
    }

    #[test]
    fn test_parameters_chained() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(1);
        let backend = JsonBackendBuilder::new()
            .working_dir(dir_path.clone())
            .snapshot_max_count(1234)
            .build();
        let builder = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Ignored)
            .kvs_load(KvsLoad::Ignored)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().instance_id, instance_id);
        assert_eq!(kvs.parameters().defaults, KvsDefaults::Ignored);
        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Ignored);
        assert!(kvs.parameters().backend.dyn_eq(
            &JsonBackendBuilder::new()
                .working_dir(dir_path)
                .snapshot_max_count(1234)
                .build()
        ));
    }

    #[test]
    fn test_build_ok() {
        let _lock = lock_and_reset();

        let instance_id = InstanceId(1);
        let builder = KvsBuilder::new(instance_id);
        let _ = builder.build().unwrap();
    }

    #[test]
    fn test_build_instance_exists_same_params() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        // Create two instances with same parameters.
        let instance_id = InstanceId(1);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let builder1 = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Ignored)
            .kvs_load(KvsLoad::Ignored)
            .backend(Box::new(backend.clone()));
        let _ = builder1.build().unwrap();

        let builder2 = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Ignored)
            .kvs_load(KvsLoad::Ignored)
            .backend(Box::new(backend));
        let kvs = builder2.build().unwrap();

        assert_eq!(kvs.parameters().instance_id, instance_id);
        assert_eq!(kvs.parameters().defaults, KvsDefaults::Ignored);
        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Ignored);
        assert!(kvs
            .parameters()
            .backend
            .dyn_eq(&JsonBackendBuilder::new().working_dir(dir_path).build()));
    }

    #[test]
    fn test_build_instance_exists_different_params() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        // Create two instances with different parameters.
        let instance_id = InstanceId(1);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let builder1 = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Ignored)
            .kvs_load(KvsLoad::Optional)
            .backend(Box::new(backend.clone()));
        let _ = builder1.build().unwrap();

        let builder2 = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Optional)
            .kvs_load(KvsLoad::Ignored)
            .backend(Box::new(backend.clone()));
        let result = builder2.build();

        assert!(result.is_err_and(|e| e == ErrorCode::InstanceParametersMismatch));
    }

    #[test]
    fn test_build_instance_exists_params_not_set() {
        let _lock = lock_and_reset();

        // Create two instances, first with parameters, second without.
        let instance_id = InstanceId(1);
        let builder1 = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Ignored)
            .kvs_load(KvsLoad::Ignored);
        let _ = builder1.build().unwrap();

        let builder2 = KvsBuilder::new(instance_id);
        let kvs = builder2.build().unwrap();

        // Assert params as expected.
        assert_eq!(kvs.parameters().instance_id, instance_id);
        assert_eq!(kvs.parameters().defaults, KvsDefaults::Ignored);
        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Ignored);
    }

    #[test]
    fn test_build_instance_exists_single_matching_param_set() {
        let _lock = lock_and_reset();

        // Create two instances, first with parameters, second only with `defaults`.
        let instance_id = InstanceId(1);
        let builder1 = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Ignored)
            .kvs_load(KvsLoad::Ignored);
        let _ = builder1.build().unwrap();

        let builder2 = KvsBuilder::new(instance_id).defaults(KvsDefaults::Ignored);
        let kvs = builder2.build().unwrap();

        // Assert params as expected.
        assert_eq!(kvs.parameters().instance_id, instance_id);
        assert_eq!(kvs.parameters().defaults, KvsDefaults::Ignored);
        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Ignored);
    }

    #[test]
    fn test_build_instance_exists_single_mismatched_param_set() {
        let _lock = lock_and_reset();

        // Create two instances, first with parameters, second only with `defaults`.
        let instance_id = InstanceId(1);
        let builder1 = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Ignored)
            .kvs_load(KvsLoad::Ignored);
        let _ = builder1.build().unwrap();

        let builder2 = KvsBuilder::new(instance_id).defaults(KvsDefaults::Optional);
        let result = builder2.build();

        assert!(result.is_err_and(|e| e == ErrorCode::InstanceParametersMismatch));
    }

    #[test]
    fn test_build_instance_id_out_of_range() {
        let _lock = lock_and_reset();

        let instance_id = InstanceId(123);
        let result = KvsBuilder::new(instance_id).build();
        assert!(result.is_err_and(|e| e == ErrorCode::InvalidInstanceId));
    }

    /// Generate and store file containing example default values.
    fn create_defaults_file(working_dir: &Path, instance_id: InstanceId) -> Result<(), ErrorCode> {
        let backend = JsonBackendBuilder::new().working_dir(working_dir.to_path_buf()).build();
        let defaults_file_path = backend.defaults_file_path(instance_id);
        let defaults_hash_file_path = backend.defaults_hash_file_path(instance_id);

        let kvs_map = KvsMap::from([
            ("number1".to_string(), KvsValue::F64(123.0)),
            ("bool1".to_string(), KvsValue::Boolean(true)),
            ("string1".to_string(), KvsValue::String("Hello".to_string())),
        ]);
        JsonBackend::save(&kvs_map, &defaults_file_path, &defaults_hash_file_path)?;

        Ok(())
    }

    /// Generate and store files containing example KVS and hash data.
    fn create_kvs_files(
        working_dir: &Path,
        instance_id: InstanceId,
        snapshot_id: SnapshotId,
    ) -> Result<(PathBuf, PathBuf), ErrorCode> {
        let backend = JsonBackendBuilder::new().working_dir(working_dir.to_path_buf()).build();
        let kvs_file_path = backend.kvs_file_path(instance_id, snapshot_id);
        let hash_file_path = backend.hash_file_path(instance_id, snapshot_id);
        let kvs_map = KvsMap::from([
            ("number1".to_string(), KvsValue::F64(321.0)),
            ("bool1".to_string(), KvsValue::Boolean(false)),
            ("string1".to_string(), KvsValue::String("Hi".to_string())),
        ]);
        JsonBackend::save(&kvs_map, &kvs_file_path, &hash_file_path)?;

        Ok((kvs_file_path, hash_file_path))
    }

    #[test]
    fn test_build_defaults_ignored() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        create_defaults_file(&dir_path, instance_id).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Ignored)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().defaults, KvsDefaults::Ignored);
        let kvs_pool = KVS_POOL.lock().unwrap();
        let kvs_pool_entry = kvs_pool.get(2).unwrap();
        let kvs_data = kvs_pool_entry.as_ref().unwrap();
        assert_eq!(kvs_data.data.lock().unwrap().defaults_map, KvsMap::new());
    }

    #[test]
    fn test_build_defaults_optional_not_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let builder = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Optional)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().defaults, KvsDefaults::Optional);
        let kvs_pool = KVS_POOL.lock().unwrap();
        let kvs_pool_entry = kvs_pool.get(2).unwrap();
        let kvs_data = kvs_pool_entry.as_ref().unwrap();
        assert_eq!(kvs_data.data.lock().unwrap().defaults_map, KvsMap::new());
    }

    #[test]
    fn test_build_defaults_optional_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        create_defaults_file(&dir_path, instance_id).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Optional)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().defaults, KvsDefaults::Optional);
        let kvs_pool = KVS_POOL.lock().unwrap();
        let kvs_pool_entry = kvs_pool.get(2).unwrap();
        let kvs_data = kvs_pool_entry.as_ref().unwrap();
        assert_eq!(kvs_data.data.lock().unwrap().defaults_map.len(), 3);
    }

    #[test]
    fn test_build_defaults_required_not_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let builder = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Required)
            .backend(Box::new(backend));
        let result = builder.build();

        assert!(result.is_err_and(|e| e == ErrorCode::FileNotFound));
    }

    #[test]
    fn test_build_defaults_required_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        create_defaults_file(&dir_path, instance_id).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .defaults(KvsDefaults::Required)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().defaults, KvsDefaults::Required);
        let kvs_pool = KVS_POOL.lock().unwrap();
        let kvs_pool_entry = kvs_pool.get(2).unwrap();
        let kvs_data = kvs_pool_entry.as_ref().unwrap();
        assert_eq!(kvs_data.data.lock().unwrap().defaults_map.len(), 3);
    }

    #[test]
    fn test_build_kvs_load_ignored() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        create_kvs_files(&dir_path, instance_id, SnapshotId(0)).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Ignored)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Ignored);
        let kvs_pool = KVS_POOL.lock().unwrap();
        let kvs_pool_entry = kvs_pool.get(2).unwrap();
        let kvs_data = kvs_pool_entry.as_ref().unwrap();
        assert_eq!(kvs_data.data.lock().unwrap().kvs_map, KvsMap::new());
    }

    #[test]
    fn test_build_kvs_load_optional_not_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Optional)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Optional);
        let kvs_pool = KVS_POOL.lock().unwrap();
        let kvs_pool_entry = kvs_pool.get(2).unwrap();
        let kvs_data = kvs_pool_entry.as_ref().unwrap();
        assert_eq!(kvs_data.data.lock().unwrap().kvs_map, KvsMap::new());
    }

    #[test]
    #[ignore = "Not handled properly yet"]
    fn test_build_kvs_load_optional_kvs_provided_hash_not_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let (_kvs_path, hash_path) = create_kvs_files(&dir_path, instance_id, SnapshotId(0)).unwrap();
        std::fs::remove_file(hash_path).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Optional)
            .backend(Box::new(backend));
        let result = builder.build();

        assert!(result.is_err_and(|e| e == ErrorCode::KvsHashFileReadError));
    }

    #[test]
    #[ignore = "Not handled properly yet"]
    fn test_build_kvs_load_optional_kvs_not_provided_hash_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let (kvs_path, _hash_path) = create_kvs_files(&dir_path, instance_id, SnapshotId(0)).unwrap();
        std::fs::remove_file(kvs_path).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Optional)
            .backend(Box::new(backend));
        let result = builder.build();

        assert!(result.is_err_and(|e| e == ErrorCode::FileNotFound));
    }

    #[test]
    fn test_build_kvs_load_optional_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        create_kvs_files(&dir_path, instance_id, SnapshotId(0)).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Optional)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Optional);
        let kvs_pool = KVS_POOL.lock().unwrap();
        let kvs_pool_entry = kvs_pool.get(2).unwrap();
        let kvs_data = kvs_pool_entry.as_ref().unwrap();
        assert_eq!(kvs_data.data.lock().unwrap().kvs_map.len(), 3);
    }

    #[test]
    fn test_build_kvs_load_required_not_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Required)
            .backend(Box::new(backend));
        let result = builder.build();

        assert!(result.is_err_and(|e| e == ErrorCode::FileNotFound));
    }

    #[test]
    #[ignore = "Not handled properly yet"]
    fn test_build_kvs_load_required_kvs_provided_hash_not_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let (_kvs_path, hash_path) = create_kvs_files(&dir_path, instance_id, SnapshotId(0)).unwrap();
        std::fs::remove_file(hash_path).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Required)
            .backend(Box::new(backend));
        let result = builder.build();

        assert!(result.is_err_and(|e| e == ErrorCode::KvsHashFileReadError));
    }

    #[test]
    #[ignore = "Not handled properly yet"]
    fn test_build_kvs_load_required_kvs_not_provided_hash_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        let (kvs_path, _hash_path) = create_kvs_files(&dir_path, instance_id, SnapshotId(0)).unwrap();
        std::fs::remove_file(kvs_path).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Required)
            .backend(Box::new(backend));
        let result = builder.build();

        assert!(result.is_err_and(|e| e == ErrorCode::FileNotFound));
    }

    #[test]
    fn test_build_kvs_load_required_provided() {
        let _lock = lock_and_reset();

        let dir = tempdir().unwrap();
        let dir_path = dir.path().to_path_buf();

        let instance_id = InstanceId(2);
        let backend = JsonBackendBuilder::new().working_dir(dir_path.clone()).build();
        create_kvs_files(&dir_path, instance_id, SnapshotId(0)).unwrap();
        let builder = KvsBuilder::new(instance_id)
            .kvs_load(KvsLoad::Required)
            .backend(Box::new(backend));
        let kvs = builder.build().unwrap();

        assert_eq!(kvs.parameters().kvs_load, KvsLoad::Required);
        let kvs_pool = KVS_POOL.lock().unwrap();
        let kvs_pool_entry = kvs_pool.get(2).unwrap();
        let kvs_data = kvs_pool_entry.as_ref().unwrap();
        assert_eq!(kvs_data.data.lock().unwrap().kvs_map.len(), 3);
    }
}

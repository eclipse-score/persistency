use crate::kvs_value::{KvsMap, KvsValue};

use crate::error_code::ErrorCode;

use crate::json_backend::json_value::{KvsJson, TinyJson};

use std::fs;

// Unified error type for all backend and JSON errors
#[derive(Debug)]
pub enum KvsBackendError {
    Io(std::io::Error),
    Json(String),
    ValidationFailed,
    MutexLockFailed,
    KeyNotFound,
    ConversionFailed,
    UnmappedError,
}

impl std::fmt::Display for KvsBackendError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            KvsBackendError::Io(e) => write!(f, "IO error: {e}"),
            KvsBackendError::Json(e) => write!(f, "JSON error: {e}"),
            KvsBackendError::ValidationFailed => write!(f, "Validation failed"),
            KvsBackendError::MutexLockFailed => write!(f, "Mutex lock failed"),
            KvsBackendError::KeyNotFound => write!(f, "Key not found"),
            KvsBackendError::ConversionFailed => write!(f, "Conversion failed"),
            KvsBackendError::UnmappedError => write!(f, "Unmapped error"),
        }
    }
}

impl std::error::Error for KvsBackendError {}

impl From<std::io::Error> for KvsBackendError {
    fn from(e: std::io::Error) -> Self {
        KvsBackendError::Io(e)
    }
}

// Trait for persisting and loading KvsMap from storage
pub trait PersistKvs {
    fn get_kvs_from_file(filename: &str, kvs: &mut KvsMap) -> Result<(), KvsBackendError>;
    fn persist_kvs_to_file(kvs: &KvsMap, filename: &str) -> Result<(), KvsBackendError>;
}

#[derive(Default)]
pub struct DefaultPersistKvs<J: KvsJson = TinyJson> {
    _marker: std::marker::PhantomData<J>,
}

impl<J: KvsJson> PersistKvs for DefaultPersistKvs<J> {
    fn get_kvs_from_file(filename: &str, kvs: &mut KvsMap) -> Result<(), KvsBackendError> {
        let data = fs::read_to_string(filename).map_err(KvsBackendError::from)?;
        let json_val = J::parse(&data).map_err(|e| KvsBackendError::Json(e.to_string()))?;
        let kvs_val = J::to_kvs_value(json_val);
        if let KvsValue::Object(map) = kvs_val {
            kvs.clear();
            kvs.extend(map);
            Ok(())
        } else {
            Err(KvsBackendError::UnmappedError)
        }
    }

    fn persist_kvs_to_file(kvs: &KvsMap, filename: &str) -> Result<(), KvsBackendError> {
        let kvs_val = KvsValue::Object(kvs.clone());
        let json_val = J::from_kvs_value(&kvs_val);
        let json_str = J::stringify(&json_val).map_err(|e| KvsBackendError::Json(e.to_string()))?;
        fs::write(filename, json_str).map_err(KvsBackendError::from)?;
        Ok(())
    }
}

impl From<KvsBackendError> for ErrorCode {
    fn from(e: KvsBackendError) -> Self {
        match e {
            KvsBackendError::Io(_) => ErrorCode::KvsFileReadError,
            KvsBackendError::Json(_) => ErrorCode::JsonParserError,
            KvsBackendError::ValidationFailed => ErrorCode::ValidationFailed,
            KvsBackendError::MutexLockFailed => ErrorCode::MutexLockFailed,
            KvsBackendError::KeyNotFound => ErrorCode::KeyNotFound,
            KvsBackendError::ConversionFailed => ErrorCode::ConversionFailed,
            KvsBackendError::UnmappedError => ErrorCode::UnmappedError,
        }
    }
}

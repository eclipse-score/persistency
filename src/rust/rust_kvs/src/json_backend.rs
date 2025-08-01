// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0

use crate::error_code::ErrorCode;
use crate::kvs_backend::KvsBackend;
use crate::kvs_value::{KvsMap, KvsValue};
use std::fs;
use std::path::PathBuf;

// for creating jsonvalue obj
use std::collections::HashMap;

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
                if let (Some(JsonValue::String(type_str)), Some(value)) =
                    (obj.remove("t"), obj.remove("v"))
                {
                    return match (type_str.as_str(), value) {
                        ("i32", JsonValue::Number(v)) => KvsValue::I32(v as i32),
                        ("u32", JsonValue::Number(v)) => KvsValue::U32(v as u32),
                        ("i64", JsonValue::Number(v)) => KvsValue::I64(v as i64),
                        ("u64", JsonValue::Number(v)) => KvsValue::U64(v as u64),
                        ("f64", JsonValue::Number(v)) => KvsValue::F64(v),
                        ("bool", JsonValue::Boolean(v)) => KvsValue::Boolean(v),
                        ("str", JsonValue::String(v)) => KvsValue::String(v),
                        ("null", JsonValue::Null) => KvsValue::Null,
                        ("arr", JsonValue::Array(v)) => {
                            KvsValue::Array(v.into_iter().map(KvsValue::from).collect())
                        }
                        ("obj", JsonValue::Object(v)) => KvsValue::Object(
                            v.into_iter().map(|(k, v)| (k, KvsValue::from(v))).collect(),
                        ),
                        // Remaining types can be handled with Null.
                        _ => KvsValue::Null,
                    };
                }
                // If not a t-tagged object, treat as a map of key-value pairs (KvsMap)
                let map: KvsMap = obj
                    .into_iter()
                    .map(|(k, v)| (k, KvsValue::from(v)))
                    .collect();
                KvsValue::Object(map)
            }
            JsonValue::Array(arr) => KvsValue::Array(arr.into_iter().map(KvsValue::from).collect()),
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
            }
            KvsValue::U32(n) => {
                obj.insert("t".to_string(), JsonValue::String("u32".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n as f64));
            }
            KvsValue::I64(n) => {
                obj.insert("t".to_string(), JsonValue::String("i64".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n as f64));
            }
            KvsValue::U64(n) => {
                obj.insert("t".to_string(), JsonValue::String("u64".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n as f64));
            }
            KvsValue::F64(n) => {
                obj.insert("t".to_string(), JsonValue::String("f64".to_string()));
                obj.insert("v".to_string(), JsonValue::Number(n));
            }
            KvsValue::Boolean(b) => {
                obj.insert("t".to_string(), JsonValue::String("bool".to_string()));
                obj.insert("v".to_string(), JsonValue::Boolean(b));
            }
            KvsValue::String(s) => {
                obj.insert("t".to_string(), JsonValue::String("str".to_string()));
                obj.insert("v".to_string(), JsonValue::String(s));
            }
            KvsValue::Null => {
                obj.insert("t".to_string(), JsonValue::String("null".to_string()));
                obj.insert("v".to_string(), JsonValue::Null);
            }
            KvsValue::Array(arr) => {
                obj.insert("t".to_string(), JsonValue::String("arr".to_string()));
                obj.insert(
                    "v".to_string(),
                    JsonValue::Array(arr.into_iter().map(JsonValue::from).collect()),
                );
            }
            KvsValue::Object(map) => {
                obj.insert("t".to_string(), JsonValue::String("obj".to_string()));
                obj.insert(
                    "v".to_string(),
                    JsonValue::Object(
                        map.into_iter()
                            .map(|(k, v)| (k, JsonValue::from(v)))
                            .collect(),
                    ),
                );
            }
        }
        JsonValue::Object(obj)
    }
}

/// tinyjson::JsonParseError -> ErrorCode::JsonParseError
impl From<JsonParseError> for ErrorCode {
    fn from(cause: JsonParseError) -> Self {
        eprintln!(
            "error: JSON parser error: line = {}, column = {}",
            cause.line(),
            cause.column()
        );
        ErrorCode::JsonParserError
    }
}

/// tinyjson::JsonGenerateError -> ErrorCode::JsonGenerateError
impl From<JsonGenerateError> for ErrorCode {
    fn from(cause: JsonGenerateError) -> Self {
        eprintln!("error: JSON generator error: msg = {}", cause.message());
        ErrorCode::JsonGeneratorError
    }
}

/// KVS backend implementation based on TinyJSON.
pub struct JsonBackend;

impl JsonBackend {
    fn parse(s: &str) -> Result<JsonValue, ErrorCode> {
        s.parse()
            .map_err(|_e: JsonParseError| crate::error_code::ErrorCode::JsonParserError)
    }

    fn stringify(val: &JsonValue) -> Result<String, ErrorCode> {
        val.stringify()
            .map_err(|_e: JsonGenerateError| crate::error_code::ErrorCode::JsonParserError)
    }
}

impl KvsBackend for JsonBackend {
    fn load_kvs(
        source_path: PathBuf,
        verify_hash: bool,
        hash_source: Option<PathBuf>,
    ) -> Result<KvsMap, ErrorCode> {
        let filename = source_path.with_extension("json");
        let data = fs::read_to_string(&filename).map_err(|_| ErrorCode::KvsFileReadError)?;
        let json_value = Self::parse(&data).map_err(|_| ErrorCode::JsonParserError)?;

        // Hash check logic (use parsed data)
        if verify_hash {
            if let Some(hash_filename) = hash_source {
                if !hash_filename.as_os_str().is_empty() {
                    match fs::read(&hash_filename) {
                        Ok(hash_bytes) => {
                            let hash_kvs =
                                adler32::RollingAdler32::from_buffer(data.as_bytes()).hash();
                            if hash_bytes.len() == 4 {
                                let file_hash = u32::from_be_bytes([
                                    hash_bytes[0],
                                    hash_bytes[1],
                                    hash_bytes[2],
                                    hash_bytes[3],
                                ]);
                                if hash_kvs != file_hash {
                                    return Err(ErrorCode::ValidationFailed);
                                }
                            } else {
                                return Err(ErrorCode::ValidationFailed);
                            }
                        }
                        Err(_) => {
                            return Err(ErrorCode::KvsHashFileReadError);
                        }
                    }
                }
            }
        }

        let kvs_value = KvsValue::from(json_value);
        if let KvsValue::Object(kvs_map) = kvs_value {
            Ok(kvs_map)
        } else {
            Err(ErrorCode::JsonParserError)
        }
    }

    fn save_kvs(kvs: &KvsMap, destination_path: PathBuf, add_hash: bool) -> Result<(), ErrorCode> {
        let filename = destination_path
            .with_extension("json")
            .with_file_name(format!("{}_0.json", destination_path.display()));

        let kvs_value = KvsValue::Object(kvs.clone());
        let json_value = JsonValue::from(kvs_value);

        let json_str = Self::stringify(&json_value).map_err(|_| ErrorCode::JsonParserError)?;
        fs::write(&filename, &json_str).map_err(|_| ErrorCode::KvsFileReadError)?;

        if add_hash {
            // Compute hash and write to hash file
            let hash = adler32::RollingAdler32::from_buffer(json_str.as_bytes()).hash();
            // If filename ends with .json, replace with .hash
            let filename_hash = if let Some(stem) = filename.file_stem() {
                let mut hash_path = filename.clone();
                hash_path.set_file_name(format!("{}.hash", stem.to_string_lossy()));
                hash_path
            } else {
                let mut hash_path = filename.clone();
                hash_path.set_extension("hash");
                hash_path
            };
            fs::write(&filename_hash, hash.to_be_bytes())
                .map_err(|_| ErrorCode::KvsFileReadError)?;
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_unknown_error_code_from_json_parse_error() {
        let error = tinyjson::JsonParser::new("[1, 2, 3".chars())
            .parse()
            .unwrap_err();
        assert_eq!(ErrorCode::from(error), ErrorCode::JsonParserError);
    }

    #[test]
    fn test_unknown_error_code_from_json_generate_error() {
        let data: JsonValue = JsonValue::Number(f64::INFINITY);
        let error = data.stringify().unwrap_err();
        assert_eq!(ErrorCode::from(error), ErrorCode::JsonGeneratorError);
    }
}

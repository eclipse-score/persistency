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
use crate::json_backend::json_value::{KvsJson, TinyJson};
use crate::kvs_value::{KvsMap, KvsValue};

use std::fs;
use std::path::PathBuf;

// Backend-agnostic trait for persisting and loading KvsMap from any source/sink
pub trait PersistKvs {
    fn load_kvs(
        source: PathBuf,
        kvs: &mut KvsMap,
        verify_hash: bool,
        hash_source: Option<PathBuf>,
    ) -> Result<(), ErrorCode>;

    fn save_kvs(kvs: &KvsMap, sink: PathBuf, add_hash: bool) -> Result<(), ErrorCode>;
}

#[derive(Default)]
pub struct DefaultPersistKvs<J: KvsJson = TinyJson> {
    _marker: std::marker::PhantomData<J>,
}

impl<J: KvsJson> PersistKvs for DefaultPersistKvs<J> {
    fn load_kvs(
        source: PathBuf,
        kvs: &mut KvsMap,
        verify_hash: bool,
        hash_source: Option<PathBuf>,
    ) -> Result<(), ErrorCode> {
        let filename = source.with_extension("json");
        let data = fs::read_to_string(&filename).map_err(|_| ErrorCode::KvsFileReadError)?;
        let json_val = J::parse(&data).map_err(|_| ErrorCode::JsonParserError)?;
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
        let kvs_val = J::to_kvs_value(json_val);
        if let KvsValue::Object(map) = kvs_val {
            kvs.clear();
            kvs.extend(map);
            Ok(())
        } else {
            Err(ErrorCode::JsonParserError)
        }
    }

    fn save_kvs(kvs: &KvsMap, sink: PathBuf, add_hash: bool) -> Result<(), ErrorCode> {
        let filename = sink.with_extension("json");
        let filename = filename.with_file_name(format!("{}_0.json", sink.display()));
        let kvs_val = KvsValue::Object(kvs.clone());
        let json_val = J::from_kvs_value(&kvs_val);
        let json_str = J::stringify(&json_val).map_err(|_| ErrorCode::JsonParserError)?;
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

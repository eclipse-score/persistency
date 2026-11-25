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
//! # Key-Value-Storage API and Implementation
//!
//! ## Introduction
//!
//! This crate provides a Key-Value-Store using [TinyJSON](https://crates.io/crates/tinyjson) to
//! persist the data. To validate the stored data a hash is build and verified using the
//! [Adler32](https://crates.io/crates/adler32) crate. No other direct dependencies are used
//! besides the Rust `std` library.
//!
//! The key-value-storage is opened or initialized with [`KvsBuilder::new`] where various settings
//! can be applied before the KVS instance is created.
//!
//! The following [`KvsValue`] variants are supported:
//!   * `U8`, `U16`, `U32`, `U64`: `u8`, `u16`, `u32`, `u64`
//!   * `I8`, `I16`, `I32`, `I64`: `i8`, `i16`, `i32`, `i64`
//!   * `F32`, `F64`: `f32`, `f64`
//!   * `Boolean`: `bool`
//!   * `String`: `String`
//!   * `Null`: `()`
//!   * `Array`: `Vec<KvsValue>`
//!   * `Object`: `HashMap<String, KvsValue>`
//!
//! Each value is stored with a type tag, so it is loaded as the same variant.
//! `TinyJSON` holds every number as `f64`, so `u64` and `i64` values are exact only up to 2^53 in magnitude.
//!
//! Note: JSON arrays are not restricted to only contain values of the same type.
//!
//! Writing a value to the KVS can be done by calling [`Kvs::set_value`] with the `key` as first and a `KvsValue` as second parameter.
//! Either `KvsValue::F64(123.0)` or `123.0` can be used, as the value is converted with `Into` when calling the function.
//! The Rust type of the value selects the variant, e.g. `123.0_f32` is stored as `F32` and `123_u8` as `U8`.
//!
//! To read a value call [`Kvs::get_value`](Kvs::get_value) or [`Kvs::get_value_as::<T>`](Kvs::get_value_as) with the `key` as first parameter.
//! `T` is the type to read and can be any of the types listed above or `KvsValue`.
//! `T` must match the stored variant, no numeric conversion is performed.
//! Also `let value: f64 = kvs.get_value_as()` can be used.
//!
//! If a `key` isn't available in the KVS a lookup into the defaults storage will be performed and
//! if the `value` is found the default will be returned. The default value isn't stored when
//! [`Kvs::flush`] is called unless it's explicitly written with [`Kvs::set_value`]. So when
//! defaults change always the latest values will be returned. If that is an unwanted behaviour
//! it's better to remove the default value and write the value permanently when the KVS is
//! initialized. To check whether a value has a default call [`Kvs::get_default_value`] and to
//! see if the value wasn't written yet and will return the default call
//! [`Kvs::is_value_default`].
//!
//!
//! ## Example Usage
//!
//! ```
//! use rust_kvs::prelude::*;
//! use std::collections::HashMap;
//!
//! fn main() -> Result<(), ErrorCode> {
//!     let kvs: Kvs = KvsBuilder::new(InstanceId(0))
//!         .build()?;
//!
//!     kvs.set_value("number", 123.0)?;
//!     kvs.set_value("bool", true)?;
//!     kvs.set_value("string", "First".to_string())?;
//!     kvs.set_value("null", ())?;
//!     kvs.set_value(
//!         "array",
//!         vec![
//!             KvsValue::from(456.0),
//!             false.into(),
//!             "Second".to_string().into(),
//!         ],
//!     )?;
//!     kvs.set_value(
//!         "object",
//!         HashMap::from([
//!             (String::from("sub-number"), KvsValue::from(789.0)),
//!             ("sub-bool".into(), true.into()),
//!             ("sub-string".into(), "Third".to_string().into()),
//!             ("sub-null".into(), ().into()),
//!             (
//!                 "sub-array".into(),
//!                 KvsValue::from(vec![
//!                     KvsValue::from(1246.0),
//!                     false.into(),
//!                     "Fourth".to_string().into(),
//!                 ]),
//!             ),
//!         ]),
//!     )?;
//!
//!     Ok(())
//! }
//! ```
//!
//! ## Feature Coverage
//!
//! Feature and requirement definition:
//!   * [Features/Persistency/Key-Value-Storage](https://github.com/eclipse-score/score/blob/ulhu_persistency_kvs/docs/features/persistency/key-value-storage/index.rst#specification)
//!   * [Requirements/Stakeholder](https://github.com/eclipse-score/score/blob/ulhu_persistency_kvs/docs/requirements/stakeholder/index.rst)
//!
//! Supported features and requirements:
//!   * `FEAT_REQ__KVS__thread_safety`
//!   * `FEAT_REQ__KVS__supported_datatypes_keys`
//!   * `FEAT_REQ__KVS__supported_datatypes_values`
//!   * `FEAT_REQ__KVS__default_values`
//!   * `FEAT_REQ__KVS__update_mechanism`: JSON format-flexibility
//!   * `FEAT_REQ__KVS__snapshots`
//!   * `FEAT_REQ__KVS__default_value_reset`
//!   * `FEAT_REQ__KVS__default_value_retrieval`
//!   * `FEAT_REQ__KVS__persistency`
//!   * `FEAT_REQ__KVS__integrity_check`
//!   * `STKH_REQ__30`: JSON storage format
//!   * `STKH_REQ__8`: Defaults stored in JSON format
//!   * `STKH_REQ__12`: Support storing data on non-volatile memory
//!   * `STKH_REQ__13`: POSIX portability
//!
//! Currently unsupported features:
//!   * `FEAT_REQ__KVS__maximum_size`
//!   * `FEAT_REQ__KVS__cpp_rust_interoperability`
//!   * `FEAT_REQ__KVS__versioning`: JSON version ID
//!   * `FEAT_REQ__KVS__tooling`: Get/set CLI, JSON editor
//!   * `STKH_REQ__350`: Safe key-value-store
//!
//! Additional info:
//!   * Feature `FEAT_REQ__KVS__supported_datatypes_keys` is matched by the Rust standard which
//!     defines that `String` and `str` are always valid UTF-8.
//!   * Feature `FEAT_REQ__KVS__supported_datatypes_values` is matched by using the same types that
//!     the IPC will use for the Rust implementation.
#![forbid(unsafe_code)]
#![cfg_attr(coverage_nightly, feature(coverage_attribute))]

pub mod error_code;
pub mod json_backend;
pub mod kvs;
pub mod kvs_api;
pub mod kvs_backend;
pub mod kvs_builder;
pub mod kvs_mock;
pub mod kvs_serialize;
pub mod kvs_value;
mod log;
#[cfg(test)]
mod test_utils;

/// Prelude module for convenient imports
pub mod prelude {
    pub use crate::error_code::ErrorCode;
    pub use crate::json_backend::{JsonBackend, JsonBackendBuilder};
    pub use crate::kvs::Kvs;
    #[allow(deprecated)]
    pub use crate::kvs_api::KVS_MAX_KEY_LENGTH_BYTES;
    pub use crate::kvs_api::{InstanceId, KvsApi, KvsDefaults, KvsLoad, SnapshotId, MAX_KEY_LENGTH_BYTES};
    pub use crate::kvs_backend::KvsBackend;
    pub use crate::kvs_builder::KvsBuilder;
    pub use crate::kvs_serialize::{KvsDeserialize, KvsSerialize};
    pub use crate::kvs_value::{KvsMap, KvsValue};
}

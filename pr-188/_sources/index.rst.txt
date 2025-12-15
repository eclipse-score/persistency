..
   # *******************************************************************************
   # Copyright (c) 2024 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************

.. toctree::
   :maxdepth: 2

   persistency/index

Key-Value-Storage (rust_kvs) Documentation
==========================================

This documentation describes the `rust_kvs` crate, which provides a key-value storage implementation with JSON-like persistence using Rust.

Summary
-------

**Crate:** `rust_kvs`

**Purpose:** Key-Value-Storage API and Implementation

**Description:**  
This crate provides a Key-Value-Store using TinyJSON to persist the data. It uses the Adler32 crate to validate stored data and depends only on the Rust standard library.

Introduction
------------

The key-value store is initialized with `Kvs::open` and can be flushed with `Kvs::flush`.

All TinyJSON-supported datatypes are available:

- `Number`: `f64`
- `Boolean`: `bool`
- `String`: `String`
- `Null`: `()`
- `Array`: `Vec<KvsValue>`
- `Object`: `HashMap<String, KvsValue>`

JSON arrays can hold mixed types.

Usage Notes:

- Use `Kvs::set_value(key, value)` to write.
- Use `Kvs::get_value::<T>(key)` to read.
- If a key is missing, the store checks for a default and returns it.
- Defaults are not flushed unless explicitly written.

To check for defaults:

- `Kvs::get_default_value`
- `Kvs::is_value_default`

Example
-------

.. code-block:: rust

   use rust_kvs::{ErrorCode, InstanceId, Kvs, OpenNeedDefaults, OpenNeedKvs, KvsValue};
   use std::collections::HashMap;

   fn main() -> Result<(), ErrorCode> {
       let kvs = Kvs::open(
           InstanceId::new(0),
           OpenNeedDefaults::Optional,
           OpenNeedKvs::Optional)?;

       kvs.set_value("number", 123.0)?;
       kvs.set_value("bool", true)?;
       kvs.set_value("string", "First".to_string())?;
       kvs.set_value("null", ())?;
       kvs.set_value(
           "array",
           vec![
               KvsValue::from(456.0),
               false.into(),
               "Second".to_string().into(),
           ],
       )?;
       kvs.set_value(
           "object",
           HashMap::from([
               ("sub-number".into(), KvsValue::from(789.0)),
               ("sub-bool".into(), true.into()),
               ("sub-string".into(), "Third".to_string().into()),
               ("sub-null".into(), ().into()),
               (
                   "sub-array".into(),
                   KvsValue::from(vec![
                       KvsValue::from(1246.0),
                       false.into(),
                       "Fourth".to_string().into(),
                   ]),
               ),
           ]),
       )?;

       Ok(())
   }

- :need:`feat_req__persistency__tooling`
- :need:`feat_req__persistency__variant_management`
- :need:`feat_req__persistency__fast_access`

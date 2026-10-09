<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->


# Getting Started with Key-Value-Storage (persistency)

This guide helps you to get started with the C++ and Rust implementations of the Key-Value-Storage (KVS)
and to integrate it with Bazel. The complete description of the module is available in the
[Persistency user manual](../docs/module/manuals/user_manual.rst).

## 1. Integrating with Bazel

### 1.1 Add the dependency to your MODULE.bazel

```python
bazel_dep(name = "score_persistency", version = "0.3.5")
```

The toolchains for C++ and Rust are set up as described in the
[S-CORE user guide](https://eclipse-score.github.io/score/main/users_guide/building_simple_application/first_score_module.html). The toolchain
configurations used by persistency itself are defined in its `.bazelrc` (`per-x86_64-linux`, `per-x86_64-qnx`,
`per-arm64-qnx`).

### 1.2 Insert this into your .bazelrc

```
build --@score_logging//score/mw/log/flags:KRemote_Logging=False
build --@score_baselibs//score/log_rust:safety_level=qm
```

### 1.3 Reference the library in your BUILD file

```python
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = ["@score_persistency//score/kvs"],  # version 0.3.5: "@score_persistency//score/kvs:kvs_cpp"
)

rust_binary(
    name = "my_rust_app",
    srcs = ["main.rs"],
    deps = ["@score_persistency//score/kvs/rust_kvs"],
)
```

## 2. Using the C++ Implementation

The C++ API is centered around the `KvsBuilder` and `Kvs` classes (namespace `score::mw::per::kvs`):

```cpp
#include "score/kvs/kvsbuilder.hpp"

#include <iostream>
#include <string>
#include <variant>

using namespace score::mw::per::kvs;

int main()
{
    auto open_res = KvsBuilder(InstanceId(0))
                        .need_defaults_flag(OpenNeedDefaults::Optional)
                        .need_kvs_flag(OpenNeedKvs::Optional)
                        .dir("./data_folder/")
                        .build();
    if (!open_res)
    {
        std::cerr << "Failed to open KVS" << std::endl;
        return 1;
    }
    Kvs kvs = std::move(open_res.value());

    // Set a key-value pair; errors shall be handled by the application (see safety manual)
    if (!kvs.set_value("username", KvsValue("alice")))
    {
        std::cerr << "Failed to set value" << std::endl;
        return 1;
    }

    // Read a stored value (returns KeyNotFound if no value is stored for the key)
    auto get_res = kvs.get_value("username");
    if (get_res && get_res.value().getType() == KvsValue::Type::String)
    {
        std::cout << "username: " << std::get<std::string>(get_res.value().getValue()) << std::endl;
    }

    // Read a default value
    auto default_res = kvs.get_default_value("language");

    // Check if a value is stored for a key
    if (kvs.key_exists("username").value_or(false))
    {
        std::cout << "username exists" << std::endl;
    }

    // Remove a key
    if (!kvs.remove_key("username"))
    {
        std::cerr << "Failed to remove key" << std::endl;
    }

    // List all keys with a stored value (defaults are not listed)
    auto keys_res = kvs.get_all_keys();

    // Write the values to the storage
    if (!kvs.flush())
    {
        std::cerr << "Failed to flush KVS" << std::endl;
        return 1;
    }

    return 0;
}
```

Further usage patterns are shown in the C++ test scenarios in `score/kvs/tests/test_scenarios/cpp`.

## 3. Using the Rust Implementation

### 3.1 Basic Usage

Based on `score/kvs/rust_kvs/examples/basic.rs`:

```rust
use rust_kvs::prelude::*;
use std::path::PathBuf;

fn main() -> Result<(), ErrorCode> {
    let kvs = KvsBuilder::new(InstanceId(0))
        .kvs_load(KvsLoad::Optional)
        .backend(Box::new(
            JsonBackendBuilder::new().working_dir(PathBuf::from("./data_folder")).build(),
        ))
        .build()?;

    kvs.set_value("number", 123.0)?;
    kvs.set_value("bool", true)?;
    kvs.set_value("string", "First")?;

    let value = kvs.get_value("number")?;
    println!("number = {value:?}");

    kvs.flush()?;
    Ok(())
}
```

### 3.2 Snapshots

Based on `score/kvs/rust_kvs/examples/snapshots.rs`:

```rust
let max_count = kvs.snapshot_max_count() as u32;
for index in 0..max_count {
    kvs.set_value("counter", index)?;
    kvs.flush()?;
    println!("Snapshot count: {:?}", kvs.snapshot_count());
}

// Restore a snapshot
kvs.snapshot_restore(SnapshotId(2))?;
```

### 3.3 Default Values

Based on `score/kvs/rust_kvs/examples/defaults.rs`:

```rust
let kvs = KvsBuilder::new(instance_id)
    .backend(Box::new(JsonBackendBuilder::new().working_dir(dir_path).build()))
    .defaults(KvsDefaults::Required)
    .build()?;

let k1_value = kvs.get_default_value("k1")?;
println!("k1 = {k1_value:?}");
```

The examples are executed with `cargo run -p rust_kvs --example basic` (accordingly for `snapshots`, `defaults`,
`custom_types` and `migration`).

## 4. Default Value File

The default values of instance `<instance id>` are read from `kvs_<instance id>_default.json` in the storage
directory. Each value is stored with its type (`t`) and value (`v`):

```json
{
    "language": {"t": "str", "v": "en"},
    "theme": {"t": "str", "v": "dark"},
    "timeout": {"t": "i32", "v": 30}
}
```

Supported types are `i32`, `u32`, `i64`, `u64`, `f64`, `bool`, `str`, `null`, `arr` and `obj`.

**Important:**
- If the KVS is opened with `OpenNeedDefaults::Required` (C++) or `KvsDefaults::Required` (Rust), the file must exist.
- The checksum file `kvs_<instance id>_default.hash` must exist alongside the defaults file. It contains the
  Adler-32 checksum of the file content as 4 bytes in big-endian order. `create_defaults_file` in
  `score/kvs/rust_kvs/examples/defaults.rs` shows how both files are created.
- Default values are returned by `get_default_value`; `get_value` returns only stored values.

## 5. Command Line Tool

`kvs_tool` (`score/kvs/rust_kvs_tool`) reads and modifies the data of KVS instance 0 in a directory, e.g.
`kvs_tool -o setkey -k MyKey -p 15 -d ./data_folder`. Run `kvs_tool --help` for all operations.


# Getting Started with Key-Value-Storage (persistency)

This guide demonstrates how to use the Key-Value-Storage (KVS) library in a standalone Bazel workspace. The example module shows how to integrate `score_persistency` with modern C++ toolchains and build a simple KVS application.

## 1. About this Example Module

This example workspace demonstrates a minimal setup for using the `score_persistency` library. It's configured as a standalone Bazel module with the following features:

### Module Structure
- **MODULE.bazel**: Defines dependencies and toolchain configuration
- **.bazelrc**: Contains build configurations for different target platforms
- **src/main.cpp**: Simple example application using the KVS C++ API
- **src/BUILD**: Bazel build target for the example

### Supported Toolchains
The example is configured to build with three different toolchains:
- **x86_64-gcc**: GCC for Linux x86_64 (POSIX)
- **x86_64-qcc**: QNX compiler for x86_64
- **aarch64-qcc**: QNX compiler for ARM64/AArch64

## 2. Bazel Configuration Explained

### 2.1 Module Dependencies (MODULE.bazel)

The [MODULE.bazel](MODULE.bazel) file defines the workspace and its dependencies:

**Core Dependencies:**
- `rules_cc`: C++ build rules
- `score_bazel_platforms`: Platform definitions for different OS/CPU combinations
- `score_bazel_cpp_toolchains`: GCC and QCC toolchain support
- `score_persistency`: The KVS library itself
- `score_baselibs`: Common utilities and result types

*See [MODULE.bazel](MODULE.bazel) for specific versions currently in use.*

**Toolchain Configuration:**
The module uses the `gcc` extension from `score_bazel_cpp_toolchains` to configure three toolchains with automatic package downloading enabled (`use_default_package = True`). This means the toolchain packages are downloaded automatically during the build.

**Local Override:**
The example uses `local_path_override` to reference the parent persistency directory, allowing testing against the latest local version instead of a published release.

### 2.2 Build Configuration (.bazelrc)

The [.bazelrc](.bazelrc) file configures how Bazel builds for different platforms:

**Registry Configuration:**
Specifies the S-CORE Bazel registry and Bazel Central Registry for downloading dependencies.

**Build Configurations:**
- **x86_64-gcc**: Linux x86_64 target with GCC, includes pthread support
- **x86_64-qcc**: QNX x86_64 target with QCC
- **aarch64-qcc**: QNX ARM64 target with QCC

**Common Build Flags:**
- Uses nlohmann JSON library via `score_baselibs`
- Disables remote logging for the KVS logger

### 2.3 Build Target (src/BUILD)

The [src/BUILD](src/BUILD) file defines a simple binary target:
- **Name**: `hello_kvs_world`
- **Source**: `main.cpp`
- **Dependencies**: `@score_persistency//:kvs_cpp`

The target is minimal and focused on demonstrating basic KVS usage.

## 3. Using the C++ Implementation

### 3.1 Example Application

The [src/main.cpp](src/main.cpp) demonstrates basic KVS operations:

- Building the KVS instance
- Show some key operations
- Error handling

### 3.2 Building the Example

To build for different platforms:

```sh
# within the examples folder (not the top level persistency folder)
# Linux x86_64 with GCC
bazel build --config=x86_64-gcc //src:hello_kvs_world

# QNX x86_64
bazel build --config=x86_64-qcc //src:hello_kvs_world

# QNX ARM64
bazel build --config=aarch64-qcc //src:hello_kvs_world
```

## 3. Using the Rust Implementation

### 3.1 Basic Usage

From `examples/basic.rs`:
```rust
use rust_kvs::prelude::*;
use std::collections::HashMap;
use tempfile::tempdir;

fn main() -> Result<(), ErrorCode> {
    let dir = tempdir()?;
    let dir_string = dir.path().to_string_lossy().to_string();
    let instance_id = InstanceId(0);

    // Build KVS instance
    let builder = KvsBuilder::new(instance_id)
        .dir(dir_string.clone())
        .kvs_load(KvsLoad::Optional);
    let kvs = builder.build()?;

    // Set values
    kvs.set_value("number", 123.0)?;
    kvs.set_value("bool", true)?;
    kvs.set_value("string", "First")?;

    // Get value
    let value = kvs.get_value("number")?;
    println!("number = {:?}", value);

    Ok(())
}
```

### 3.2 Snapshots Example

From `examples/snapshots.rs`:
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

### 3.3 Defaults Example

From `examples/defaults.rs`:
```rust
// Create defaults file and build KVS instance with defaults
create_defaults_file(dir.path().to_path_buf(), instance_id)?;
let builder = KvsBuilder::new(instance_id)
    .dir(dir_string)
    .defaults(KvsDefaults::Required);
let kvs = builder.build()?;

// Get default value
let k1_value = kvs.get_default_value("k1")?;
println!("k1 = {:?}", k1_value);
```
## 4. Default value file example
This file should be placed in the working directory:
```json
{
    "language": "en",
    "theme": "dark",
    "timeout": 30
}
```

**Important:**
- If you open the KVS with `.need_defaults_flag(true)`, the file must exist.
- The KVS will use these defaults for any key not explicitly set.
- You must also provide a CRC file (e.g., `defaults.json.crc`) alongside the defaults file. This CRC file is generated using the Adler-32 checksum algorithm, as implemented in the codebase. The CRC ensures the integrity of the defaults file at runtime.
## 5. More Examples
- See `src/cpp/tests/` for C++ test scenarios and usage patterns.
- See `src/rust/rust_kvs/examples/` for Rust usage patterns.

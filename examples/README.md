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

Add a `bazel_dep` on the module `score_persistency` with one of the versions listed in the
[S-CORE Bazel registry](https://github.com/eclipse-score/bazel_registry/tree/main/modules/score_persistency).
Changes between the versions are documented in the [release note](../docs/module/release/release_note.rst).

The toolchains for C++ and Rust are set up as described in the
[S-CORE user guide](https://eclipse-score.github.io/score/main/users_guide/building_simple_application/first_score_module.html).
The toolchain configurations used by persistency itself are defined in its [.bazelrc](../.bazelrc).

### 1.2 Set the build flags in your .bazelrc

The flags of `score_logging` and `score_baselibs` in the section "Default build flags (all configurations)" of
persistency's [.bazelrc](../.bazelrc) have to be set in your `.bazelrc` as well.

### 1.3 Reference the library in your BUILD file

```python
cc_binary(
    name = "my_app",
    srcs = ["main.cpp"],
    deps = ["@score_persistency//score/kvs"],
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
#include "score/kvs/kvs_builder.h"

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

Further usage patterns are shown in the C++ test scenarios in [score/kvs/tests/test_scenarios/cpp](../score/kvs/tests/test_scenarios/cpp).

## 3. Using the Rust Implementation

The Rust examples are located in [score/kvs/rust_kvs/examples](../score/kvs/rust_kvs/examples). The content of each
example is described in its header comment:

- [basic.rs](../score/kvs/rust_kvs/examples/basic.rs)
- [defaults.rs](../score/kvs/rust_kvs/examples/defaults.rs)
- [snapshots.rs](../score/kvs/rust_kvs/examples/snapshots.rs)
- [custom_types.rs](../score/kvs/rust_kvs/examples/custom_types.rs)
- [migration.rs](../score/kvs/rust_kvs/examples/migration.rs)

An example is executed with `cargo run -p rust_kvs --example <file name without .rs>`, e.g.
`cargo run -p rust_kvs --example basic`.

Further usage patterns are shown in the Rust test scenarios in
[score/kvs/tests/test_scenarios/rust](../score/kvs/tests/test_scenarios/rust).

## 4. Default Value File

The configuration of default values and the checksum of the default value file are specified in the component
requirements `comp_req__kvs__default_value_cfg` and `comp_req__kvs__default_val_chksum`
([KVS requirements](../score/kvs/docs/requirements/index.rst)). How the default value file and its checksum file
are created is shown by `create_defaults_file` in [defaults.rs](../score/kvs/rust_kvs/examples/defaults.rs).

## 5. Command Line Tool

`kvs_tool` ([score/kvs/rust_kvs_tool](../score/kvs/rust_kvs_tool)) reads and modifies the data of a KVS instance.
Run `kvs_tool --help` for all operations.

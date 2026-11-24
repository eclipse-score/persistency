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

The C++ API is centered around the `KvsBuilder` and `Kvs` classes (namespace `score::kvs`):

```cpp
#include "score/kvs/kvs_builder.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <variant>

using namespace score::kvs;

int main()
{
    const InstanceId instance_id{0U};
    auto open_res{KvsBuilder{instance_id}
                      .Defaults(KvsDefaults::kOptional)
                      .KvsLoad(KvsLoad::kOptional)
                      .WorkingDir("./data_folder/")
                      .Build()};
    if (!open_res)
    {
        std::cerr << "Failed to open KVS" << std::endl;
        return 1;
    }
    std::shared_ptr<Kvs> kvs{std::move(open_res.value())};

    // Set a key-value pair; errors shall be handled by the application (see safety manual).
    if (!kvs->SetValue("username", KvsValue("alice")))
    {
        std::cerr << "Failed to set value" << std::endl;
        return 1;
    }

    // Read a stored value (returns kKeyNotFound if no value is stored for the key)
    auto get_res{kvs->GetValue("username")};
    if (!get_res)
    {
        std::cerr << "Failed to get value" << std::endl;
        return 1;
    }
    if (get_res.value().GetType() == KvsValue::Type::kString)
    {
        std::cout << "username: " << std::get<std::string>(get_res.value().GetValue()) << std::endl;
    }

    // Check if a value is stored for a key
    auto exists_res{kvs->KeyExists("username")};
    if (!exists_res)
    {
        std::cerr << "Failed to check key" << std::endl;
        return 1;
    }
    if (exists_res.value())
    {
        std::cout << "username exists" << std::endl;
    }

    // List all keys with a stored value (defaults are not listed)
    auto keys_res{kvs->GetAllKeys()};
    if (!keys_res)
    {
        std::cerr << "Failed to list keys" << std::endl;
        return 1;
    }
    for (const auto& key : keys_res.value())
    {
        std::cout << "key: " << key << std::endl;
    }

    // Remove a key
    if (!kvs->RemoveKey("username"))
    {
        std::cerr << "Failed to remove key" << std::endl;
        return 1;
    }

    // Write the values to the storage
    if (!kvs->Flush())
    {
        std::cerr << "Failed to flush KVS" << std::endl;
        return 1;
    }

    // Release the application handle before unregistering the instance.
    kvs.reset();
    auto unregister_res{Unregister(instance_id)};
    if (!unregister_res)
    {
        std::cerr << "Failed to unregister KVS" << std::endl;
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

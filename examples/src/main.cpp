/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#include <iostream>
#include <kvsbuilder.hpp>

int main() {
    std::cout << "Hello, score_persistency world!" << std::endl;

    auto open_res = score::mw::per::kvs::KvsBuilder(score::mw::per::kvs::InstanceId(0))
        .need_defaults_flag(false)
        .need_kvs_flag(false)
        .dir(".")
        .build();

    if (!open_res) {
        std::cerr << "Failed to open KVS: " << open_res.error() << std::endl;
        return 1;
    }
    score::mw::per::kvs::Kvs kvs = std::move(open_res.value());

    // Set a key-value pair
    kvs.set_value("username", score::mw::per::kvs::KvsValue("alice"));

    // Read a value
    score::Result<score::mw::per::kvs::KvsValue> get_res = kvs.get_value("username");
    if (get_res) {
        std::cout << "username: " << std::get<std::string>(get_res.value().getValue()) << std::endl;
    }
    return 0;
}

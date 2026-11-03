# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
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
load("@rules_rust//rust:defs.bzl", "rust_binary")

rust_binary(
    name = "rust_test_utility",
    srcs = ["main.rs"],
    crate_features = ["score_log_bridge"],
    crate_name = "rust_test_utility",
    visibility = ["//score/kvs/rust_kvs_tool/tests:__subpackages__"],
    deps = ["//score/kvs/rust_kvs"],
)

# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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
    name = "kvs_tool",
    srcs = [
        "src/kvs_tool.rs",
    ],
    crate_features = ["score_log_bridge"],
    crate_name = "rust_kvs_tool",
    rustc_flags = [
        "-Clink-arg=-lstdc++",
        "-Clink-arg=-lm",
        "-Clink-arg=-lc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/rust/rust_kvs",
        "@score_baselibs_rust//src/log/score_log",
        "@score_crates//:pico_args",
        "@score_crates//:tinyjson",
        "@score_logging//score/mw/log/rust/score_log_bridge",
    ],
)

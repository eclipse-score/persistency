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
load("@rules_rust//rust:defs.bzl", "miri_binary", "rust_binary")

rust_binary(
    name = "kvs_tool",
    srcs = ["kvs_tool.rs"],
    crate_features = ["score_log_bridge"],
    visibility = ["//visibility:public"],
    deps = [
        "//score/kvs_rust",
        "@score_baselibs//score/log_rust/score_log",
        "@score_crates//:pico_args",
        "@score_crates//:tinyjson",
        "@score_logging//score/mw/log/rust/score_log_bridge",
    ],
)

miri_binary(
    name = "kvs_tool_miri",
    crate = ":kvs_tool",
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

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
    name = "kvs_persist_reload",
    srcs = ["kvs_persist_reload.rs"],
    crate_features = ["score_log_bridge"],
    rustc_flags = [
        "-Clink-arg=-lstdc++",
        "-Clink-arg=-lm",
        "-Clink-arg=-lc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//score/kvs/rust_kvs",
        "@score_baselibs//score/log_rust/score_log",
        "@score_crates//:tempfile",
        "@score_crates//:tracing",
        "@score_crates//:tracing_subscriber",
        "@score_logging//score/mw/log/rust/score_log_bridge",
    ],
)

rust_binary(
    name = "kvs_default_values",
    srcs = ["kvs_default_values.rs"],
    crate_features = ["score_log_bridge"],
    rustc_flags = [
        "-Clink-arg=-lstdc++",
        "-Clink-arg=-lm",
        "-Clink-arg=-lc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//score/kvs/rust_kvs",
        "@score_baselibs//score/log_rust/score_log",
        "@score_crates//:adler32",
        "@score_crates//:tempfile",
        "@score_crates//:tinyjson",
        "@score_crates//:tracing",
        "@score_crates//:tracing_subscriber",
        "@score_logging//score/mw/log/rust/score_log_bridge",
    ],
)

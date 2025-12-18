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
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

rust_library(
    name = "rust_kvs",
    srcs = glob(["src/**/*.rs"]),
    visibility = ["//visibility:public"],
    deps = [
        "@score_crates//:adler32",
        "@score_crates//:tinyjson",
    ],
)

rust_test(
    name = "tests",
    crate = "rust_kvs",
    tags = [
        "unit_tests",
        "ut",
    ],
    deps = ["@score_crates//:tempfile"],
)

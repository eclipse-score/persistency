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
load("@rules_rust//rust:defs.bzl", "miri_test", "rust_library", "rust_test")

rust_library(
    name = "rust_kvs",
    srcs = glob(["src/**/*.rs"]),
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs_rust//src/log/score_log",
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

miri_test(
    name = "tests_miri",
    crate = ":tests",
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

miri_test(
    name = "tests_miri_error_code",
    crate = ":tests",
    miri_args = ["error_code::"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

miri_test(
    name = "tests_miri_json_backend",
    crate = ":tests",
    miri_args = ["json_backend::"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

miri_test(
    name = "tests_miri_kvs",
    crate = ":tests",
    miri_args = ["kvs::"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

miri_test(
    name = "tests_miri_kvs_api",
    crate = ":tests",
    miri_args = ["kvs_api::"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

miri_test(
    name = "tests_miri_kvs_builder",
    crate = ":tests",
    miri_args = ["kvs_builder::"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

miri_test(
    name = "tests_miri_kvs_mock",
    crate = ":tests",
    miri_args = ["kvs_mock::"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

miri_test(
    name = "tests_miri_kvs_serialize",
    crate = ":tests",
    miri_args = ["kvs_serialize::"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

miri_test(
    name = "tests_miri_kvs_value",
    crate = ":tests",
    miri_args = ["kvs_value::"],
    tags = ["manual"],
    visibility = ["//visibility:public"],
)

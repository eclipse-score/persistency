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

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@score_docs_as_code//:docs.bzl", "docs_bundle")

docs_bundle(
    name = "kvs_docs",
    code_targets = [
        ":kvs",
        "//score/kvs/rust_kvs:rust_kvs",
    ],
    primary_need_id = "comp__persistency_kvs",
    source_dir = "docs",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "error",
    srcs = ["error.cpp"],
    hdrs = ["error.hpp"],
    visibility = ["//visibility:public"],
    deps = [
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "kvs_config_parser",
    srcs = [
        "internal/kvs_config_parser.cpp",
    ],
    hdrs = [
        "internal/ikvs_config_parser.hpp",
        "internal/kvs_config_parser.hpp",
        "internal/kvs_config_parser_factory.hpp",
        "internal/kvs_config_types.hpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":error",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/json",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:stdlib",
        "@score_baselibs//score/result",
    ],
)

# Defines the production `create_kvs_config_parser()` symbol. Kept separate from
# `:kvs_config_parser` so that `:kvs_core` can depend on the parser implementation/types without
# linking a factory definition; only `:kvs` (production) or `:kvs_config_parser_mock` (tests) is
# ever linked alongside `:kvs_core`, never both factory definitions together.
# `alwayslink` is required because `:kvs_core` calls `create_kvs_config_parser()` without
# depending on this target, so a statically-linked binary cannot rely on archive member ordering
# to pull this object file in (the single-pass linker would otherwise drop it as unreferenced).
cc_library(
    name = "kvs_config_parser_factory",
    srcs = ["internal/kvs_config_parser_factory.cpp"],
    hdrs = ["internal/kvs_config_parser_factory.hpp"],
    visibility = ["//visibility:private"],
    deps = [
        ":kvs_config_parser",
    ],
    alwayslink = True,
)

cc_library(
    name = "kvs_config_parser_mock",
    testonly = True,
    srcs = ["internal/kvs_config_parser_mock_factory.cpp"],
    hdrs = [
        "internal/ikvs_config_parser.hpp",
        "internal/kvs_config_parser_factory.hpp",
        "internal/kvs_config_parser_mock.hpp",
        "internal/kvs_config_types.hpp",
    ],
    visibility = ["//:__pkg__"],
    deps = [
        ":error",
        "@googletest//:gtest",
        "@score_baselibs//score/result",
    ],
    alwayslink = True,
)

# Core KVS implementation, deliberately without a linked `create_kvs_config_parser()` definition.
# `:unit_tests` links this together with `:kvs_config_parser_mock`; `:kvs` (below) links it
# together with `:kvs_config_parser_factory` for production use. Never link both factory targets
# alongside this library in the same binary (see `kvs_config_parser_factory.hpp`).
cc_library(
    name = "kvs_core",
    srcs = [
        "internal/kvs_helper.cpp",
        "internal/kvs_helper.hpp",
        "kvs.cpp",
        "kvsbuilder.cpp",
        "kvsvalue.cpp",
    ],
    hdrs = [
        "kvs.hpp",
        "kvsbuilder.hpp",
        "kvsvalue.hpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":error",
        ":kvs_config_parser",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/json",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log",
        "@score_baselibs//score/os:errno",
        "@score_baselibs//score/os:fcntl",
        "@score_baselibs//score/os:stat",
        "@score_baselibs//score/os:stdio",
        "@score_baselibs//score/os:unistd",
        "@score_baselibs//score/result",
    ],
)

# Public production target: `:kvs_core` plus the real `create_kvs_config_parser()` definition.
cc_library(
    name = "kvs",
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":kvs_config_parser_factory",
        ":kvs_core",
    ],
)

alias(
    name = "kvs_cpp",
    actual = "//score/kvs",
    deprecation = "//score/kvs:kvs_cpp has moved to //score/kvs " +
                  "Update your dependency to the //score/kvs label.",
    visibility = ["//visibility:public"],
)

cc_test(
    name = "unit_tests",
    size = "small",
    srcs = [
        "kvs_builder_test.cpp",
        "kvs_error_test.cpp",
        "kvs_general_test.cpp",
        "kvs_general_test.hpp",
        "kvs_helper_test.cpp",
        "kvs_test.cpp",
        "kvs_value_test.cpp",
    ],
    visibility = ["//:__pkg__"],
    deps = [
        ":kvs_config_parser_mock",
        ":kvs_core",
        "@googletest//:gtest_main",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/filesystem:mock",
        "@score_baselibs//score/json:mock",
        "@score_baselibs//score/language/futurecpp",
        "@score_baselibs//score/mw/log:backend_stub_testutil",
        "@score_baselibs//score/os:object_seam",
        "@score_baselibs//score/os:stat",
        "@score_baselibs//score/os/mocklib:fcntl_mock",
        "@score_baselibs//score/os/mocklib:stat_mock",
        "@score_baselibs//score/result",
    ],
)

cc_test(
    name = "kvs_config_real_test",
    size = "small",
    testonly = True,
    srcs = [
        "kvs_config_general_test.hpp",
        "kvs_config_test.cpp",
    ],
    visibility = ["//:__pkg__"],
    deps = [
        ":kvs",
        "@googletest//:gtest_main",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/json",
        "@score_baselibs//score/result",
    ],
)

cc_test(
    name = "kvs_config_parser_test",
    size = "small",
    testonly = True,
    srcs = [
        "kvs_config_general_test.hpp",
        "kvs_config_parser_test.cpp",
    ],
    visibility = ["//:__pkg__"],
    deps = [
        ":kvs_config_parser",
        "@googletest//:gtest_main",
        "@score_baselibs//score/filesystem",
        "@score_baselibs//score/json",
        "@score_baselibs//score/result",
    ],
)

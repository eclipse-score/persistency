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

load("@score_docs_as_code//:docs.bzl", "docs")
load("@score_format_checker//:macros.bzl", "use_format_targets")
load("@score_sbom//:defs.bzl", "sbom")
load("@score_tooling//:defs.bzl", "cli_helper", "copyright_checker", "setup_starpls")

# Creates all documentation targets:
# - `:docs` for building documentation at build-time
docs(
    bundles = [
        {
            # JSON Component?
            "bundle": "//score:json_docs",
            "mount_at": "components/json",
        },
        {
            # KVS Component
            "bundle": "//score/kvs:kvs_docs",
            "mount_at": "components/kvs",
        },
    ],
    external_needs = [
        "@score_platform//:needs_json_file",
        "@score_process_description//:needs_json_file",
    ],
    project = "S-CORE persistency",
    project_url = "https://eclipse-score.github.io/persistency/",
)

setup_starpls(
    name = "starpls_server",
    visibility = ["//visibility:public"],
)

copyright_checker(
    name = "copyright",
    srcs = [
        ".github",
        "BUILD",
        "MODULE.bazel",
        "docs",
        "examples",
        "score",
        "tools",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
)

# Generates SBOMs (SPDX 2.3 + CycloneDX 1.6) for the KVS library.
# - Rust crate licenses/suppliers come from the crates.io API via
#   auto_crates_cache (network access required at build time).
# - Python dependency licenses come from dash-license-scan via the
#   python_lockfiles path (host JRE required).
sbom(
    name = "sbom",
    auto_crates_cache = True,
    cargo_lockfile = "Cargo.lock",
    component_name = "score_persistency",
    module_lockfiles = [":MODULE.bazel.lock"],
    python_lockfiles = ["//score/kvs/tests/test_cases:requirements.txt.lock"],
    targets = [
        "//score/kvs:kvs_cpp",
        "//score/kvs/rust_kvs:rust_kvs",
    ],
    visibility = ["//visibility:public"],
)

cli_helper(
    name = "cli-help",
    visibility = ["//visibility:public"],
)

exports_files(
    [
        # Used by the @score_tooling coverage reporter to locate the workspace root.
        "MODULE.bazel",
        "pyproject.toml",
    ],
)

# Add target for formatting checks
use_format_targets()

alias(
    name = "kvs_cpp",
    actual = "//score/kvs:kvs_cpp",
    tags = ["cli_help=Build KVS CPP [build]"],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "test_kvs_cpp",
    tests = ["//score/kvs/tests:test_kvs_cpp"],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "bm_kvs_cpp",
    tests = ["//score/kvs/tests:bm_kvs_cpp"],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "unit_tests",
    tests = [
        "test_kvs_cpp",
        "//score/kvs/rust_kvs:tests",
    ],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "cit_tests",
    tests = [
        "//score/kvs/tests/test_cases:cit_cpp",
        "//score/kvs/tests/test_cases:cit_rust",
    ],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "miri_tests",
    tags = ["manual"],
    tests = [
        "//score/kvs/rust_kvs:tests_miri_error_code",
        "//score/kvs/rust_kvs:tests_miri_json_backend",
        "//score/kvs/rust_kvs:tests_miri_kvs",
        "//score/kvs/rust_kvs:tests_miri_kvs_api",
        "//score/kvs/rust_kvs:tests_miri_kvs_builder",
        "//score/kvs/rust_kvs:tests_miri_kvs_mock",
        "//score/kvs/rust_kvs:tests_miri_kvs_serialize",
        "//score/kvs/rust_kvs:tests_miri_kvs_value",
    ],
    visibility = ["//visibility:public"],
)

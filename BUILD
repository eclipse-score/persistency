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
load("@score_sbom//:defs.bzl", "sbom")
load("@score_tooling//:defs.bzl", "cli_helper", "copyright_checker", "dash_license_checker", "setup_starpls")
load("@score_tooling//third_party/format:macros.bzl", "use_format_targets")

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

dash_license_checker(
    src = "//:Cargo.lock",
    file_type = "cargo",
    filter_keywords = ["github.com/eclipse-score/"],
    visibility = ["//visibility:public"],
)

# Generates the product SBOM (SPDX 2.3 + CycloneDX 1.6) for the KVS library.
# - Rust crate licenses/suppliers come from the crates.io API via
#   auto_crates_cache (network access required at build time).
# - Build-time/test tooling is covered separately by //:sbom_docs_tests.
sbom(
    name = "sbom_product",
    component_name = "score_persistency",
    module_lockfiles = [":MODULE.bazel.lock"],
    targets = [
        "//score/kvs:kvs_cpp",
        "//score/kvs/rust_kvs:rust_kvs",
    ],
    visibility = ["//visibility:public"],
)

# Qualification inventory for Python-based build and test tools. This is kept
# separate from the product SBOM because build-time dependencies are not
# product/runtime dependencies.
sbom(
    name = "sbom_docs_tests",
    testonly = True,
    component_name = "score_persistency",
    module_lockfiles = [":MODULE.bazel.lock"],
    python_lockfiles = [
        "//score/kvs/tests/test_cases:requirements.txt.lock",
        "@score_docs_as_code//src:requirements_lock",
    ],
    targets = [
        "//:docs",
        "//:unit_tests",
        "//:cit_tests",
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
    name = "unit_tests",
    tests = [
        "//score/kvs:unit_tests",
        "//score/kvs/rust_kvs:unit_tests",
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
        "//score/kvs/rust_kvs:unit_tests_miri_error_code",
        "//score/kvs/rust_kvs:unit_tests_miri_json_backend",
        "//score/kvs/rust_kvs:unit_tests_miri_kvs",
        "//score/kvs/rust_kvs:unit_tests_miri_kvs_api",
        "//score/kvs/rust_kvs:unit_tests_miri_kvs_builder",
        "//score/kvs/rust_kvs:unit_tests_miri_kvs_mock",
        "//score/kvs/rust_kvs:unit_tests_miri_kvs_serialize",
        "//score/kvs/rust_kvs:unit_tests_miri_kvs_value",
    ],
    visibility = ["//visibility:public"],
)

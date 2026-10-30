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
load("@score_tooling//:defs.bzl", "setup_starpls")
load(":qemu.bzl", "qemu_aarch64")

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

# Exported for the Dash license check in //tools.
exports_files(
    ["Cargo.lock"],
    visibility = ["//tools:__pkg__"],
)

# cicd on-pr.yml detects these root labels with `bazel query`.
# It skips a check when its root label is missing.
# The targets live in //tools.
alias(
    name = "copyright.check",
    actual = "//tools:copyright.check",
    visibility = ["//visibility:public"],
)

alias(
    name = "copyright.fix",
    actual = "//tools:copyright.fix",
    visibility = ["//visibility:public"],
)

# This is not an alias on purpose.
# `bazel test` does not expand a test_suite behind an alias.
# The cicd `bazel test //:format.check` would find no tests.
# Tagged manual so that `bazel test //...` skips the format check.
test_suite(
    name = "format.check",
    tags = ["manual"],
    tests = ["//tools:format.check"],
    visibility = ["//visibility:public"],
)

alias(
    name = "format.fix",
    actual = "//tools:format.fix",
    visibility = ["//visibility:public"],
)

# cicd license-check.yml runs `bazel run //:license-check` (see .github/workflows/license_check.yml).
# The target lives in //tools.
alias(
    name = "license-check",
    actual = "//tools:license-check",
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
        "//score/kvs:kvs",
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

# Lint config, consumed by //tools/lint:linters.bzl.
exports_files([".clang-tidy"])

exports_files(
    [
        # MODULE.bazel: the LLVM coverage pipeline (//tools/coverage) resolves the workspace root through it at report time.
        # MODULE.bazel is also the workspace marker for //tools:format.check.
        "MODULE.bazel",
        "pyproject.toml",
    ],
)

alias(
    name = "kvs_cpp",
    actual = "//score/kvs",
    deprecation = "//:kvs_cpp is deprecated. Update your dependency to the //score/kvs label.",
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
        "//score/kvs/rust_kvs/tests:cit_tests",
        "//score/kvs/tests:cit_tests",
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

# Wrapper for running aarch64 Linux test binaries under qemu-user (--config=score-linux-aarch64).
qemu_aarch64()

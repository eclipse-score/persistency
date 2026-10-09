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
load("@pip_score_persistency_venv_test//:requirements.bzl", "all_requirements")
load("@rules_python//python:pip.bzl", "compile_pip_requirements")
load("@score_tools//score_pytest:pytest.bzl", "score_pytest")

# Referenced by //:sbom (python_lockfiles).
exports_files(
    ["requirements.txt.lock"],
    visibility = ["//:__pkg__"],
)

# In order to update the requirements, change the `requirements.in` file and run:
# `bazel run //score/kvs/tests/test_cases:requirements.update`.
# This will update the `requirements.txt` file.
# To upgrade all dependencies to their latest versions, run:
# `bazel run //score/kvs/tests/test_cases:requirements.update -- --upgrade`.
compile_pip_requirements(
    name = "requirements",
    srcs = [
        "requirements.txt",
        # Keeps pytest aligned with the version score_pytest ships.
        "@score_tools//score_pytest:requirements_py312.txt",
    ],
    requirements_txt = "requirements.txt.lock",
    tags = [
        "manual",
    ],
)

filegroup(
    name = "cit_files",
    srcs = glob(["tests/**/*.py"]) + ["test_properties.py"],
    visibility = [
        "//score/kvs/tests:__subpackages__",
        "//score/kvs_rust/tests:__subpackages__",
    ],
)

# Tests targets
score_pytest(
    name = "cit_cpp",
    size = "small",
    srcs = [":cit_files"],
    args = [
        "-m cpp",
        "--cpp-target-path=$(rootpath //score/kvs/tests/test_scenarios/cpp:test_scenarios)",
        "--traces=all",
        "--log-cli-level=DEBUG",
    ],
    data = ["//score/kvs/tests/test_scenarios/cpp:test_scenarios"],
    env = {
        "RUST_BACKTRACE": "1",
    },
    pytest_config = "//:pyproject.toml",
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//:__pkg__"],
    deps = all_requirements,
)

score_pytest(
    name = "cit_rust",
    size = "small",
    srcs = [":cit_files"],
    args = [
        "-m rust",
        "--rust-target-path=$(rootpath //score/kvs/tests/test_scenarios/rust:test_scenarios)",
        "--traces=all",
        "--log-cli-level=DEBUG",
    ],
    data = ["//score/kvs/tests/test_scenarios/rust:test_scenarios"],
    env = {
        "RUST_BACKTRACE": "1",
    },
    pytest_config = "//:pyproject.toml",
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//:__pkg__"],
    deps = all_requirements,
)

test_suite(
    name = "cit",
    tests = [
        ":cit_cpp",
        ":cit_rust",
    ],
    visibility = ["//visibility:public"],
)

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
load("@rules_python//python:pip.bzl", "compile_pip_requirements")

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
        "//score/kvs/rust_kvs/tests:__subpackages__",
        "//score/kvs/tests:__subpackages__",
    ],
)

test_suite(
    name = "cit",
    deprecation = "//score/kvs/tests/test_cases:cit has moved to //:cit_tests.",
    tests = [
        "//score/kvs/rust_kvs/tests:cit_tests",
        "//score/kvs/tests:cit_tests",
    ],
    visibility = ["//visibility:public"],
)

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
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files")
load("//tools/integration_testing:integration_testing.bzl", "integration_test")

pkg_files(
    name = "test_scenarios_pkg",
    srcs = ["//score/kvs/tests/test_scenarios"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "/opt/score_persistency",
)

integration_test(
    name = "cit_tests",
    srcs = ["//score/kvs/tests/test_cases:cit_files"],
    args = [
        "-m cpp",
        "--cpp-target-path=/opt/score_persistency/test_scenarios",
        "--traces=all",
        "--log-cli-level=DEBUG",
    ],
    filesystem = ":test_scenarios_pkg",
    pytest_config = "//:pyproject.toml",
    visibility = ["//:__pkg__"],
    deps = all_requirements,
)

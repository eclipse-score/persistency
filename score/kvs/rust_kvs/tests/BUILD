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
load("@pip_score_persistency_venv_test//:requirements.bzl", "all_requirements")
load("@score_tools//score_pytest:pytest.bzl", "score_pytest")

score_pytest(
    name = "cit_tests",
    size = "small",
    srcs = ["//score/kvs/tests/test_cases:cit_files"],
    args = [
        "-m rust",
        "--rust-target-path=$(rootpath //score/kvs/rust_kvs/tests/test_scenarios)",
        "--traces=all",
        "--log-cli-level=DEBUG",
    ],
    data = ["//score/kvs/rust_kvs/tests/test_scenarios"],
    env = {
        "RUST_BACKTRACE": "1",
    },
    pytest_config = "//:pyproject.toml",
    target_compatible_with = ["@platforms//os:linux"],
    visibility = ["//:__pkg__"],
    deps = all_requirements,
)

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

load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@score_itf//:defs.bzl", "py_itf_test")

oci_image(
    "kvs_tool_tests_image",
    testonly = True,
    base = "@ubuntu_linux_amd64",
    target_compatible_with = ["@platforms//os:linux"],
)

oci_load(
    name = "kvs_tool_tests_image_load",
    testonly = True,
    image = ":kvs_tool_tests_image",
    repo_tags = ["persistency_tests:latest"],
    target_compatible_with = ["@platforms//os:linux"],
)

config_setting(
    name = "config_linux_x86_64",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
)

pkg_files(
    name = "kvs_tool_bin",
    srcs = ["//score/kvs/rust_kvs_tool:kvs_tool"],
    attributes = pkg_attributes(mode = "0755"),
    visibility = ["//visibility:private"],
)

pkg_files(
    name = "rust_test_utility_bin",
    srcs = ["//score/kvs/rust_kvs_tool/tests/rust_test_utility"],
    attributes = pkg_attributes(mode = "0755"),
    visibility = ["//visibility:private"],
)

pkg_files(
    name = "cpp_test_utility_bin",
    srcs = ["//score/kvs/rust_kvs_tool/tests/cpp_test_utility"],
    attributes = pkg_attributes(mode = "0755"),
    visibility = ["//visibility:private"],
)

pkg_tar(
    name = "kvs_tool_tests_deploy",
    srcs = [
        ":cpp_test_utility_bin",
        ":kvs_tool_bin",
        ":rust_test_utility_bin",
    ],
    visibility = ["//visibility:private"],
)

py_itf_test(
    name = "tests",
    size = "small",
    srcs = [
        "conftest.py",
        "helpers.py",
        "test_help.py",
        "test_store_load.py",
    ],
    args = select(
        {
            ":config_linux_x86_64": [
                "--docker-image-bootstrap=$(location :kvs_tool_tests_image_load)",
                "--docker-image=persistency_tests:latest",
            ],
        },
        no_match_error = "Unsupported platform",
    ) + [
        "-vvv",
        "-s",
        "--keep-target",
        "--deploy=$(rootpath //score/kvs/rust_kvs_tool/tests:kvs_tool_tests_deploy)",
    ],
    data = select(
        {":config_linux_x86_64": [":kvs_tool_tests_image_load"]},
        no_match_error = "Unsupported platform",
    ) + [
        "//score/kvs/rust_kvs_tool/tests:kvs_tool_tests_deploy",
    ],
    plugins = select(
        {":config_linux_x86_64": ["@score_itf//score/itf/plugins:docker_plugin"]},
        no_match_error = "Unsupported platform",
    ),
    tags = ["exclusive"],
    visibility = ["//visibility:public"],
)

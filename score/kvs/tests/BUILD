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
load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")
load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@score_itf//:defs.bzl", "py_itf_test")
load("@score_rules_imagefs//rules/qnx:ifs.bzl", "qnx_ifs")

# Component integration tests (C++), run with ITF on a target with the test scenarios installed:
# a Docker container on Linux, a QEMU VM on QNX.

pkg_files(
    name = "test_scenarios_pkg",
    srcs = ["//score/kvs/tests/test_scenarios"],
    attributes = pkg_attributes(mode = "0755"),
    prefix = "/opt/score_persistency",
)

pkg_tar(
    name = "test_scenarios_tar",
    testonly = True,
    srcs = [":test_scenarios_pkg"],
)

oci_image(
    name = "cit_tests_image",
    testonly = True,
    base = "@ubuntu_linux_amd64",
    target_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    tars = [":test_scenarios_tar"],
)

oci_load(
    name = "cit_tests_image_load",
    testonly = True,
    image = ":cit_tests_image",
    repo_tags = ["persistency_cit_cpp:latest"],
    target_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
)

qnx_ifs(
    name = "cit_tests_ifs",
    testonly = True,
    srcs = [
        ":test_scenarios_pkg",
        "//tools/integration_testing/environments/qnx8_qemu:qnx_config",
    ],
    out = "cit_tests.ifs",
    build_file = "//tools/integration_testing/environments/qnx8_qemu:init_x86_64.build",
    target_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:qnx",
    ],
)

py_itf_test(
    name = "cit_tests",
    size = "large",
    srcs = ["//score/kvs/tests/test_cases:cit_files"],
    args = [
        "-m cpp",
        "--cpp-target-path=/opt/score_persistency/test_scenarios",
        "--traces=all",
        "--log-cli-level=DEBUG",
        # One target for the whole session, the scenario fixtures are class-scoped.
        "--keep-target",
    ] + select({
        "@platforms//os:qnx": [
            "--qemu-config=$(location //tools/integration_testing/environments/qnx8_qemu:qemu_config.json)",
            "--qemu-kernel=$(location :cit_tests_ifs)",
        ],
        "//conditions:default": [
            "--docker-image-bootstrap=$(location :cit_tests_image_load)",
            "--docker-image=persistency_cit_cpp:latest",
        ],
    }),
    data = select({
        "@platforms//os:qnx": [
            ":cit_tests_ifs",
            "//tools/integration_testing/environments/qnx8_qemu:qemu_config.json",
        ],
        "//conditions:default": [":cit_tests_image_load"],
    }),
    plugins = ["@score_itf//score/itf/plugins:attribute_plugin"] + select({
        "@platforms//os:qnx": ["@score_itf//score/itf/plugins:qemu_plugin"],
        "//conditions:default": ["@score_itf//score/itf/plugins:docker_plugin"],
    }),
    pytest_config = "//:pyproject.toml",
    # block-network: a private network namespace, so parallel QEMU VMs don't clash on the forwarded SSH port.
    # integration-test: excluded from coverage, see quality/coverage/coverage.bazelrc.
    tags = [
        "block-network",
        "integration-test",
    ],
    target_compatible_with = ["@platforms//cpu:x86_64"] + select({
        "@platforms//os:qnx": ["@platforms//os:qnx"],
        "//conditions:default": ["@platforms//os:linux"],
    }),
    visibility = ["//:__pkg__"],
    deps = all_requirements,
)

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
"""Integration tests run with ITF (https://github.com/eclipse-score/itf)."""

load("@rules_oci//oci:defs.bzl", "oci_image", "oci_load")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@score_itf//:defs.bzl", "py_itf_test")
load("@score_rules_imagefs//rules/qnx:ifs.bzl", "qnx_ifs")

visibility(["//..."])

_QNX_ENVIRONMENT = "//tools/integration_testing/environments/qnx8_qemu"

def integration_test(name, srcs, filesystem, **kwargs):
    """Runs pytest `srcs` with ITF on a target that has `filesystem` installed.

    The target follows the configured platform:
    - Linux: a Docker container started from Ubuntu 24.04 with `filesystem` on top.
    - QNX: a QEMU VM booting a QNX IFS image with `filesystem` included.

    A single target is shared by the whole test session (`--keep-target`).

    Args:
        name: test target name.
        srcs: pytest source files.
        filesystem: a `pkg_*` target installed into the target filesystem.
        **kwargs: forwarded to `py_itf_test`.
    """

    # Docker image names must be unique per test, tests can run in parallel.
    repo_tag = "{}/{}:latest".format(native.package_name(), name)
    linux_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ]
    qnx_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:qnx",
    ]

    pkg_tar(
        name = "_{}_filesystem".format(name),
        srcs = [filesystem],
        testonly = True,
    )

    oci_image(
        name = "_{}_image".format(name),
        base = "@ubuntu_linux_amd64",
        tars = [":_{}_filesystem".format(name)],
        target_compatible_with = linux_compatible_with,
        testonly = True,
    )

    oci_load(
        name = "_{}_image_load".format(name),
        image = ":_{}_image".format(name),
        repo_tags = [repo_tag],
        target_compatible_with = linux_compatible_with,
        testonly = True,
    )

    qnx_ifs(
        name = "_{}_ifs".format(name),
        out = "{}.ifs".format(name),
        build_file = _QNX_ENVIRONMENT + ":init_x86_64.build",
        srcs = [filesystem, _QNX_ENVIRONMENT + ":qnx_config"],
        target_compatible_with = qnx_compatible_with,
        testonly = True,
    )

    kwargs["data"] = kwargs.get("data", []) + select({
        "@platforms//os:qnx": [
            ":_{}_ifs".format(name),
            _QNX_ENVIRONMENT + ":qemu_config.json",
        ],
        "//conditions:default": [":_{}_image_load".format(name)],
    })
    kwargs["args"] = kwargs.get("args", []) + [
        "--keep-target",
    ] + select({
        "@platforms//os:qnx": [
            "--qemu-config=$(location {}:qemu_config.json)".format(_QNX_ENVIRONMENT),
            "--qemu-kernel=$(location :_{}_ifs)".format(name),
        ],
        "//conditions:default": [
            "--docker-image-bootstrap=$(location :_{}_image_load)".format(name),
            "--docker-image={}".format(repo_tag),
        ],
    })
    kwargs["target_compatible_with"] = kwargs.get("target_compatible_with", []) + select({
        "@platforms//os:qnx": qnx_compatible_with,
        "//conditions:default": linux_compatible_with,
    })

    # The target is a whole container or VM.
    kwargs.setdefault("size", "large")
    kwargs.setdefault("timeout", "moderate")

    # A private network namespace per test, so parallel QEMU targets don't clash
    # on the forwarded SSH port. Docker is reached through its unix socket.
    # 'integration-test' keeps these tests out of coverage runs (quality/coverage/coverage.bazelrc).
    kwargs["tags"] = kwargs.get("tags", []) + ["block-network", "integration-test"]

    py_itf_test(
        name = name,
        srcs = srcs,
        plugins = [
            "@score_itf//score/itf/plugins:attribute_plugin",
        ] + select({
            "@platforms//os:qnx": ["@score_itf//score/itf/plugins:qemu_plugin"],
            "//conditions:default": ["@score_itf//score/itf/plugins:docker_plugin"],
        }),
        **kwargs
    )

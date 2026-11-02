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

load("@rules_pkg//pkg:mappings.bzl", "pkg_files")

# QNX 8 VM run by the ITF qemu plugin, see //tools/integration_testing:integration_testing.bzl.

exports_files(
    [
        "init_x86_64.build",
        "qemu_config.json",
    ],
    visibility = ["//:__subpackages__"],
)

pkg_files(
    name = "qnx_config",
    srcs = ["qcrypto.conf"],
    prefix = "/etc",
    visibility = ["//:__subpackages__"],
)

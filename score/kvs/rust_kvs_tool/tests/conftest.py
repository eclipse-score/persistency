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


"""
ITF configuration shared by the `kvs_tool` tests.
"""


def pytest_addoption(parser):
    parser.addoption("--deploy", required=True, help="Runfiles path of the deploy tarball.")
    parser.addoption(
        "--target-tmp-dir",
        default="/tmp",
        help="Writable directory on the target for the uploaded deployment and its logs. "
        "A QNX target's /tmp is /dev/shmem, which holds no directories, so it has to name a real filesystem.",
    )

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
`--help` command test.
"""

from pathlib import Path

from helpers import execute, text


def test_help(target, deployed_package: Path):
    out = execute(target, f"exec {deployed_package}/kvs_tool --help")
    assert "KVS Tool - Command Line Interface" in text(out)

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
Bi-directional load/store tests.
- `test_store` - create data using `createtestdata`, check if it's readable by both C++ and Rust implementations.
- `test_load` - create data using C++ and Rust implementations, check if it's readable by KVS tool.
"""

from pathlib import Path

import pytest
from helpers import execute


@pytest.mark.parametrize("impl", ["cpp", "rust"])
def test_store(target, deployed_package: Path, impl: str, data_dir: Path):
    execute(target, f"exec {deployed_package}/kvs_tool -o createtestdata -d {data_dir}")

    out = execute(target, f"exec {deployed_package}/{impl}_test_utility store {data_dir}")
    assert "PASSED" in out


@pytest.mark.parametrize("impl", ["cpp", "rust"])
def test_load(target, deployed_package: Path, impl: str, data_dir: Path):
    execute(target, f"exec {deployed_package}/{impl}_test_utility load {data_dir}")

    def get_key(key: str, expected_value: str):
        out = execute(target, f"exec {deployed_package}/kvs_tool -d {data_dir} -o getkey -k {key}")
        assert f"Key '{key}' exists!" in out
        assert f"Key Value: {expected_value}" in out

    get_key("i32", "I32(-123)")
    get_key("u32", "U32(123)")
    get_key("i64", "I64(-12345)")
    get_key("u64", "U64(12345)")
    get_key("f64", "F64(12345.6)")
    get_key("bool", "Boolean(true)")
    get_key("string", 'String("string-value")')
    get_key("null", "Null")
    get_key("array", 'Array([I64(555), String("string-in-array")])')
    get_key("object", 'Object({"f64-in-object": F64(123.456)})')

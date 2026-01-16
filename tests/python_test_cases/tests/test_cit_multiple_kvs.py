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
from pathlib import Path
from typing import Any

import pytest
from testing_utils import LogContainer, ScenarioResult
from test_properties import add_test_properties 

from .common import CommonScenario, ResultCode

pytestmark = pytest.mark.parametrize("version", ["rust", "cpp"], scope="class")



@add_test_properties(
    partially_verifies=[ "comp_req__persistency__multi_instance_v2", "comp_req__persistency__concurrency_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestMultipleInstanceIds(CommonScenario):
    """Verifies that multiple KVS instances with different IDs store and retrieve independent values without interference."""
    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.multiple_kvs.multiple_instance_ids"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return {
            "kvs_parameters_1": {
                "kvs_parameters": {"instance_id": 1, "dir": str(temp_dir)}
            },
            "kvs_parameters_2": {
                "kvs_parameters": {"instance_id": 2, "dir": str(temp_dir)}
            },
        }

    def test_ok(self, results: ScenarioResult, logs_info_level: LogContainer):
        assert results.return_code == ResultCode.SUCCESS

        key = "number"
        log1 = logs_info_level.find_log("instance", value="kvs1")
        assert log1 is not None
        assert log1.key == key
        assert round(log1.value, 1) == 111.1

        log2 = logs_info_level.find_log("instance", value="kvs2")
        assert log2 is not None
        assert log2.key == key
        assert round(log2.value, 1) == 222.2



@add_test_properties(
    partially_verifies=[ "comp_req__persistency__multi_instance_v2", "comp_req__persistency__concurrency_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestSameInstanceIdSameValue(CommonScenario):
    """Checks that multiple KVS instances with the same ID and key maintain consistent values across instances."""
    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.multiple_kvs.same_instance_id_same_value"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return {"kvs_parameters": {"instance_id": 1, "dir": str(temp_dir)}}

    def test_ok(self, results: ScenarioResult, logs_info_level: LogContainer):
        assert results.return_code == ResultCode.SUCCESS

        key = "number"
        log1 = logs_info_level.find_log("instance", value="kvs1")
        assert log1 is not None
        assert log1.key == key
        assert round(log1.value, 1) == 111.1

        log2 = logs_info_level.find_log("instance", value="kvs2")
        assert log2 is not None
        assert log2.key == key
        assert round(log2.value, 1) == 111.1

        assert log1.value == log2.value



@add_test_properties(
    partially_verifies=[ "comp_req__persistency__multi_instance_v2", "comp_req__persistency__concurrency_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestSameInstanceIdDifferentValue(CommonScenario):
    """Verifies that changes in one KVS instance with a shared ID and key are reflected in another instance, demonstrating interference."""
    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.multiple_kvs.same_instance_id_diff_value"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return {"kvs_parameters": {"instance_id": 1, "dir": str(temp_dir)}}

    def test_ok(self, results: ScenarioResult, logs_info_level: LogContainer):
        assert results.return_code == ResultCode.SUCCESS

        # Assertions are same as in 'TestSameInstanceIdSameValue'.
        # Test scenario behavior differs underneath.
        key = "number"
        log1 = logs_info_level.find_log("instance", value="kvs1")
        assert log1 is not None
        assert log1.key == key
        assert round(log1.value, 1) == 222.2

        log2 = logs_info_level.find_log("instance", value="kvs2")
        assert log2 is not None
        assert log2.key == key
        assert round(log2.value, 1) == 222.2

        assert log1.value == log2.value

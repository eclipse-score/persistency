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
from common import CommonScenario, ResultCode
from test_properties import add_test_properties
from testing_utils import LogContainer, ScenarioResult

pytestmark = pytest.mark.parametrize("version", ["rust", "cpp"], scope="class")


@add_test_properties(
    partially_verifies=[
        "comp_req__kvs__multi_instance",
        "comp_req__kvs__concurrency",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
class TestMultipleInstanceIds(CommonScenario):
    """Verifies that multiple KVS instances with different IDs store and retrieve independent values.

    The instances must not interfere with each other.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.multiple_kvs.multiple_instance_ids"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return {
            "kvs_parameters_1": {"kvs_parameters": {"instance_id": 1, "dir": str(temp_dir)}},
            "kvs_parameters_2": {"kvs_parameters": {"instance_id": 2, "dir": str(temp_dir)}},
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
    partially_verifies=[
        "comp_req__kvs__multi_instance",
        "comp_req__kvs__concurrency",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
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
    partially_verifies=[
        "comp_req__kvs__multi_instance",
        "comp_req__kvs__concurrency",
    ],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
class TestSameInstanceIdDifferentValue(CommonScenario):
    """Verifies that two handles opened with the same instance ID share one in-memory state.

    A value set through the second handle is visible through the first before any flush.
    After a restart, both handles read the last written value.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.multiple_kvs.same_instance_id_diff_value"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return {"kvs_parameters": {"instance_id": 1, "dir": str(temp_dir)}}

    def test_ok(self, results: ScenarioResult, logs_info_level: LogContainer):
        assert results.return_code == ResultCode.SUCCESS

        key = "number"
        # Read through the first handle right after the second handle's write, before any flush.
        shared_log = logs_info_level.find_log("instance", value="kvs1_before_flush")
        assert shared_log is not None
        assert shared_log.key == key
        assert round(shared_log.value, 1) == 222.2

        # Values read after the restart.
        log1 = logs_info_level.find_log("instance", value="kvs1")
        assert log1 is not None
        assert log1.key == key
        assert round(log1.value, 1) == 222.2

        log2 = logs_info_level.find_log("instance", value="kvs2")
        assert log2 is not None
        assert log2.key == key
        assert round(log2.value, 1) == 222.2

        assert log1.value == log2.value

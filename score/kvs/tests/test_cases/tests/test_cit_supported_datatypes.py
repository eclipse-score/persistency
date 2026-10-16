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
import json
from abc import abstractmethod
from typing import Any

import pytest
from common import CommonScenario, ResultCode
from test_properties import add_test_properties
from testing_utils import LogContainer, ScenarioResult

pytestmark = pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")


@add_test_properties(
    partially_verifies=[
        "comp_req__kvs__key_naming",
        "comp_req__kvs__key_encoding",
        "comp_req__kvs__key_length",
    ],
    test_type="interface-test",
    derivation_technique="requirements-analysis",
)
class TestSupportedDatatypesKeys(CommonScenario):
    """
    Verifies that KVS accepts only non-empty printable ASCII keys up to 32 bytes long.
    """

    # Must match keys used by the test scenario.
    VALID_KEYS = {
        "example",
        "snake_case_key",
        "kebab-case-key",
        "MixedCase0123",
        "a",
        "k" * 32,
        "with space",
        "dot.key",
        "slash/key",
    }
    INVALID_KEYS = {
        "",
        "k" * 33,
        "emoji ✅❗😀",
        "greek ημα",
        "ą" * 16,
    }

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.supported_datatypes.keys"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {"kvs_parameters": {"instance_id": 1}}

    @staticmethod
    def _set_results(logs_info_level: LogContainer) -> dict[str, str]:
        logs = logs_info_level.get_logs(field="set_ok")
        return {log.key: log.set_ok for log in logs}

    @staticmethod
    def _stored_keys(logs_info_level: LogContainer) -> set[str]:
        logs = logs_info_level.get_logs(field="stored_key")
        return {log.stored_key for log in logs}

    def test_valid_keys_accepted(self, results: ScenarioResult, logs_info_level: LogContainer) -> None:
        assert results.return_code == ResultCode.SUCCESS

        set_results = self._set_results(logs_info_level)
        for key in self.VALID_KEYS:
            assert set_results[key] == "true", f"Valid key rejected: {key!r}"

        stored_keys = self._stored_keys(logs_info_level)
        assert stored_keys >= self.VALID_KEYS

    def test_invalid_keys_rejected(self, results: ScenarioResult, logs_info_level: LogContainer) -> None:
        assert results.return_code == ResultCode.SUCCESS

        set_results = self._set_results(logs_info_level)
        for key in self.INVALID_KEYS:
            assert set_results[key] == "false", f"Invalid key accepted: {key!r}"

        stored_keys = self._stored_keys(logs_info_level)
        assert stored_keys == self.VALID_KEYS


@add_test_properties(
    partially_verifies=[
        "comp_req__kvs__value_data_types",
    ],
    test_type="interface-test",
    derivation_technique="requirements-analysis",
)
class TestSupportedDatatypesValues(CommonScenario):
    """Verifies that KVS supports storing and retrieving values of all supported types."""

    @abstractmethod
    def exp_key(self) -> str:
        pass

    @abstractmethod
    def exp_value(self) -> Any:
        pass

    def exp_tagged(self) -> dict[str, Any]:
        return {"t": self.exp_key(), "v": self.exp_value()}

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return f"cit.supported_datatypes.values.{self.exp_key()}"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {"kvs_parameters": {"instance_id": 1}}

    def test_ok(self, results: ScenarioResult, logs_info_level: LogContainer) -> None:
        assert results.return_code == ResultCode.SUCCESS

        # Get log containing type and value.
        logs = logs_info_level.get_logs(field="key", value=self.exp_key())
        assert len(logs) == 1
        log = logs[0]

        # Assert key.
        act_key = log.key
        assert act_key == self.exp_key()

        # Assert values.
        act_value = json.loads(log.value)
        assert act_value == self.exp_tagged()


class TestSupportedDatatypesValues_I32(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "i32"

    def exp_value(self) -> Any:
        return -321


class TestSupportedDatatypesValues_U32(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "u32"

    def exp_value(self) -> Any:
        return 1234


class TestSupportedDatatypesValues_I64(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "i64"

    def exp_value(self) -> Any:
        return -123456789


class TestSupportedDatatypesValues_U64(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "u64"

    def exp_value(self) -> Any:
        return 123456789


class TestSupportedDatatypesValues_F64(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "f64"

    def exp_value(self) -> Any:
        return -5432.1


class TestSupportedDatatypesValues_Bool(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "bool"

    def exp_value(self) -> Any:
        return True


class TestSupportedDatatypesValues_String(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "str"

    def exp_value(self) -> Any:
        return "example"


class TestSupportedDatatypesValues_Array(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "arr"

    def exp_value(self) -> Any:
        return [
            {"t": "f64", "v": 321.5},
            {"t": "bool", "v": False},
            {"t": "str", "v": "hello"},
            {"t": "null", "v": None},
            {"t": "arr", "v": []},
            {
                "t": "obj",
                "v": {
                    "sub-number": {
                        "t": "f64",
                        "v": 789,
                    },
                },
            },
        ]


class TestSupportedDatatypesValues_Object(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "obj"

    def exp_value(self) -> Any:
        return {"sub-number": {"t": "f64", "v": 789}}

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
        "comp_req__persistency__key_naming_v2",
        "comp_req__persistency__key_uniqueness_v2",
    ],
    fully_verifies=[
        "comp_req__persistency__key_encoding_v2",
        "comp_req__persistency__key_length_v2",
    ],
    description="Tests that the key-value store accepts valid key formats, enforces uniqueness, encodes keys as UTF-8, and restricts key length.",
    test_type="requirements-based",
    derivation_technique="interface-test",
)
class TestSupportedDatatypesKeys(CommonScenario):
    """Verifies that KVS supports UTF-8 string keys for storing and retrieving values."""

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.supported_datatypes.keys"

    @pytest.fixture(scope="class")
    def test_config(self) -> dict[str, Any]:
        return {"kvs_parameters": {"instance_id": 1}}

    @pytest.mark.xfail(
        reason="KVS does not implement requirement #1: key character set (alphanumeric, underscore, dash) enforcement. It accepts space,special char,invalid keys."
    )
    def test_key_datatypes(self, results: ScenarioResult, logs_info_level: LogContainer) -> None:
        assert results.return_code == ResultCode.SUCCESS
        logs = logs_info_level.get_logs(field="key")
        act_keys = set(map(lambda x: x.key, logs))
        # Valid keys
        valid_keys = {
            "alphaNumeric123",
            "with_underscore",
            "with-dash",
            "A1_b2-C3",
            "a" * 32,
        }
        # Invalid keys (including non-ASCII, emoji, spaces, etc.)
        invalid_keys = {
            "has space",
            "has$pecial",
            "emoji✅",
            "too_long_key_abcdefghijklmnopqrstuvwxyz123456",
            "utf8_ключ",
            "utf8_漢字",
            "utf8_emoji ✅❗😀",
            "utf8_greek ημα",
        }
        # UTF-8 keys
        utf8_keys = {
            "utf8_emoji_valid",
            "utf8_alphaNumeric123",
            "utf8_with_underscore",
            "utf8-with-dash",
            "utf8_A1_b2-C3",
        }
        # Check valid keys are present
        assert valid_keys.issubset(act_keys)
        # Check UTF-8 keys are present
        assert utf8_keys.issubset(act_keys)
        # Check invalid keys are not present
        assert act_keys.isdisjoint(invalid_keys)

    @pytest.mark.xfail(
        reason="KVS allows duplicate keys by updating the value for an existing key instead of rejecting duplicates (requirement #2 not enforced)."
    )
    def test_duplicate_key(self, results: ScenarioResult, logs_info_level: LogContainer) -> None:
        assert results.return_code == ResultCode.SUCCESS
        # Check only one instance of duplicate key
        # Uniqueness: duplicate key is currently fulfilled by allowing update to the value for an existing key (not strict rejection).
        duplicate_key_logs = [x for x in logs if x.key == "unique_key"]
        # TODO: Is allowing value update for an existing key compliant with the uniqueness requirement?
        assert len(duplicate_key_logs) == 1, (
            "Duplicate key insertion should be rejected (only one instance allowed, no update)"
        )
        assert len(duplicate_key_logs) == 1

    @pytest.mark.xfail(reason="KVS does not enforce length limits on keys.")
    def test_max_length_key(self, results: ScenarioResult, logs_info_level: LogContainer) -> None:
        assert results.return_code == ResultCode.SUCCESS
        logs = logs_info_level.get_logs(field="key")
        act_keys = set(map(lambda x: x.key, logs))
        # Max length key
        max_length_key = "a" * 32
        # Check max length key is present
        assert max_length_key in act_keys


@add_test_properties(
    partially_verifies=[
        "comp_req__persistency__value_length_v2",
    ],
    fully_verifies=[
        "comp_req__persistency__value_data_types_v2",
        "comp_req__persistency__value_serialize_v2",
    ],
    description="Tests that the key-value store accepts only allowed value types, serializes values as JSON, and enforces value length limits.",
    test_type="requirements-based",
    derivation_technique="interface-test",
)
class TestSupportedDatatypesValues(CommonScenario):
    """Verifies that KVS supports UTF-8 string keys for storing and retrieving values."""

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


class TestSupportedDatatypesValues_String1024(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "str_1024"

    def exp_value(self) -> Any:
        return "x" * 1024


class TestSupportedDatatypesValues_String1025(TestSupportedDatatypesValues):
    def exp_key(self) -> str:
        return "str_1025"

    def exp_value(self) -> Any:
        return "y" * 1025

    @pytest.mark.xfail(reason="KVS does not yet enforce value length limit (requirement #7)")
    def test_ok(self, results: ScenarioResult, logs_info_level: LogContainer) -> None:
        # Requirement #7: The component shall limit the maximum length of a value to 1024 bytes.
        # TODO: If this assertion fails, the KVS is not enforcing requirement #7.
        # For >1024 bytes, expect error or no log
        assert results.return_code != ResultCode.SUCCESS or not logs_info_level.get_logs(
            field="key", value=self.exp_key()
        )

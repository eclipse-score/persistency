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
import json
from collections.abc import Generator
from dataclasses import dataclass
from pathlib import Path
from typing import Any
from zlib import adler32

import pytest
from common import CommonScenario, ResultCode, create_defaults_file, temp_dir_common, write_file_on_target
from score.itf.plugins.core import Target
from test_properties import add_test_properties
from testing_utils import LogContainer, ScenarioResult

INSTANCE_ID = 1

# Storage files written before the open, as an earlier process leaves them after a flush: the key "stored" = 30.
STORED_DATA = json.dumps({"t": "obj", "v": {"stored": {"t": "i32", "v": 30}}}).encode()
STORAGE_FILES = {
    f"kvs_{INSTANCE_ID}_0.json": STORED_DATA,
    f"kvs_{INSTANCE_ID}_0.hash": adler32(STORED_DATA).to_bytes(length=4, byteorder="big"),
}

# Default values: only "with_default" has one.
DEFAULTS = {"with_default": 10}

# Keys queried in every key state.
KEYS = ["stored", "with_default", "without_default"]


@dataclass(frozen=True)
class KeyState:
    """
    Operations run in this order after the open, in memory only, and the written key-value pairs in memory after them.
    """

    operations: list[tuple[Any, ...]]
    written: dict[str, int]


# Key states by name.
# Only the "stored_..." states name "stored". In the other states it stays as loaded from storage, except that "reset"
# removes all keys.
KEY_STATES = {
    "never_written": KeyState(operations=[], written={"stored": 30}),
    "set": KeyState(
        operations=[("set", "with_default", 11), ("set", "without_default", 21)],
        written={"stored": 30, "with_default": 11, "without_default": 21},
    ),
    # A written value equal to the default value.
    "set_to_default_value": KeyState(
        operations=[("set", "with_default", 10)],
        written={"stored": 30, "with_default": 10},
    ),
    "overwritten": KeyState(
        operations=[
            ("set", "with_default", 11),
            ("set", "without_default", 21),
            ("set", "with_default", 12),
            ("set", "without_default", 22),
        ],
        written={"stored": 30, "with_default": 12, "without_default": 22},
    ),
    "removed": KeyState(
        operations=[
            ("set", "with_default", 11),
            ("set", "without_default", 21),
            ("remove", "with_default"),
            ("remove", "without_default"),
        ],
        written={"stored": 30},
    ),
    "reset": KeyState(
        operations=[("set", "with_default", 11), ("set", "without_default", 21), ("reset",)],
        written={},
    ),
    # The key loaded from storage, changed in memory only: the storage still holds "stored" = 30.
    "stored_overwritten": KeyState(operations=[("set", "stored", 31)], written={"stored": 31}),
    "stored_removed": KeyState(operations=[("remove", "stored")], written={}),
}


@add_test_properties(
    fully_verifies=[
        "comp_req__kvs__key_exists_api",
        "comp_req__kvs__key_list_api",
        "comp_req__kvs__default_value_is_default_api",
        "comp_req__kvs__value_set_api",
        "comp_req__kvs__key_value_overwrite",
        "comp_req__kvs__key_remove_api",
        "comp_req__kvs__store_reset_api",
    ],
    # value_get_api is partial until eclipse-score/reference_integration#485 corrects its text;
    # then move it to fully_verifies.
    partially_verifies=[
        "comp_req__kvs__value_get_api",
        "comp_req__kvs__key_uniqueness",
        "comp_req__kvs__cached_access",
    ],
    test_type="requirements-based",
    derivation_technique="equivalence-classes",
)
@pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")
@pytest.mark.parametrize("state", list(KEY_STATES), scope="class")
class TestQueriesByKeyState(CommonScenario):
    """
    Verifies that key exists, the key list, is value default and get value report the in-memory state of a key with
    and a key without a default value that are never written, set (also to the default value), overwritten, removed
    or reset, and of a key loaded from storage that is kept, overwritten or removed in memory. Only a set or the load
    makes a key written, never its default value; remove deletes only its key, reset deletes every written key, and
    get value returns the value in memory, else an error, also for a key that has a default value.
    """

    @pytest.fixture(scope="class")
    def temp_dir(self, target: Target, version: str, state: str) -> Generator[Path, None, None]:
        yield from temp_dir_common(target, self.__class__.__name__, version, state)

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.key_value.query_after_operations"

    @pytest.fixture(scope="class")
    def test_config(self, target: Target, temp_dir: Path, state: str) -> dict[str, Any]:
        """
        Write the default values and the stored key, return the input that opens the KVS, runs the operations of the
        key state and queries the keys.
        """
        create_defaults_file(target, temp_dir, INSTANCE_ID, {key: ("i32", value) for key, value in DEFAULTS.items()})
        for name, content in STORAGE_FILES.items():
            write_file_on_target(target, temp_dir / name, content)
        return {
            "kvs_parameters": {
                "instance_id": INSTANCE_ID,
                "dir": str(temp_dir),
                "defaults": "required",
                "kvs_load": "required",
            },
            "operations": KEY_STATES[state].operations,
            "keys": KEYS,
        }

    def test_key_exists(self, state: str, results: ScenarioResult, logs_info_level: LogContainer):
        """
        Checks that key exists reports true for a written key and false for any other key, even with a default value.

        Steps:
        1. Create an empty working folder on the target.
        2. Write the default-values file "kvs_1_default.json" with the default value "with_default" = 10, and its hash
           file. Write the storage files "kvs_1_0.json" with the key "stored" = 30 and its hash file "kvs_1_0.hash",
           as an earlier process leaves them after a flush.
        3. Start the scenario program. It opens the KVS (key-value store) in this folder; the open fails if the
           default values or the stored key cannot be loaded.
        4. Run the operations of the key state under test, in memory only (no flush). If an operation fails, the
           program stops with an error.
           - "never_written": no operation;
           - "set": set "with_default" = 11 and "without_default" = 21;
           - "set_to_default_value": set "with_default" = 10, the same value as its default value;
           - "overwritten": set them as in "set", then set "with_default" = 12 and "without_default" = 22;
           - "removed": set them as in "set", then remove "with_default" and remove "without_default";
           - "reset": set them as in "set", then reset (remove all keys from memory);
           - "stored_overwritten": set "stored" = 31, while the storage files still hold "stored" = 30;
           - "stored_removed": remove "stored".
        5. Log every key of the key list (get all keys). For each of the keys "stored", "with_default" and
           "without_default", log the results of key exists, is value default and get value.
        6. Compare the logged results of key exists with the keys written in memory.

        Expected result:
        - The program ends without an error.
        - "stored" exists in every state except "reset" and "stored_removed".
        - "with_default" exists after "set", "set_to_default_value" and "overwritten" only, also when its value equals
          its default value. In the other states it does not exist, even though it has a default value.
        - "without_default" exists after "set" and "overwritten" only.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        logged = {log.key: log.key_exists for log in logs_info_level.get_logs("key")}
        assert logged == {key: "Ok(true)" if key in KEY_STATES[state].written else "Ok(false)" for key in KEYS}

    def test_key_list(self, state: str, results: ScenarioResult, logs_info_level: LogContainer):
        """
        Checks that the key list holds exactly the written keys, each once, and no key that only has a default value.

        Steps:
        1. Run steps 1 to 5 of "test_key_exists".
        2. Compare the logged key list with the keys written in memory.

        Expected result:
        - The program ends without an error.
        - "never_written", "removed" and "stored_overwritten": the list is exactly "stored", so it does not hold
          "with_default", which only has a default value, nor a removed key.
        - "set" and "overwritten": the list is exactly "stored", "with_default" and "without_default", each once, also
          after the same key was set twice.
        - "set_to_default_value": the list is exactly "stored" and "with_default", although the value of
          "with_default" equals its default value.
        - "reset": the list is empty, so reset also removed "stored", which was loaded from storage.
        - "stored_removed": the list is empty, so remove also deletes a key loaded from storage.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        listed = sorted(log.listed_key for log in logs_info_level.get_logs("listed_key"))
        assert listed == sorted(KEY_STATES[state].written)

    def test_value_is_default(self, state: str, results: ScenarioResult, logs_info_level: LogContainer):
        """
        Checks that is value default reports true exactly for a key that is not written and has a default value.

        Steps:
        1. Run steps 1 to 5 of "test_key_exists".
        2. Compare the logged results of is value default with the keys written in memory and the default values.

        Expected result:
        - The program ends without an error.
        - "with_default": false where it is written ("set", "set_to_default_value" and "overwritten"), also when its
          value equals its default value; true in every other state, where it is not written.
        - "stored" where it is written (every state except "reset" and "stored_removed") and "without_default" where
          it is written ("set" and "overwritten"): false.
        - A key that is neither written nor has a default value ("without_default" in every state except "set" and
          "overwritten"; "stored" after "reset" and "stored_removed"): false or an error, such as KeyNotFound, never
          true. Both false and an error are accepted for such a key, by project decision; the requirement does not
          say which answer to give.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        written = KEY_STATES[state].written
        # Keys that are neither written nor have a default value: false and an error are both accepted.
        no_value = {key for key in KEYS if key not in written and key not in DEFAULTS}
        logged = {
            log.key: "Ok(false) or Err"
            if log.key in no_value and (log.value_is_default == "Ok(false)" or log.value_is_default.startswith("Err("))
            else log.value_is_default
            for log in logs_info_level.get_logs("key")
        }
        # Written keys: false. Keys that are not written: true with a default value, else false or an error.
        expected = {
            key: "Ok(false)" if key in written else "Ok(true)" if key in DEFAULTS else "Ok(false) or Err"
            for key in KEYS
        }
        assert logged == expected

    def test_get_value(self, state: str, results: ScenarioResult, logs_info_level: LogContainer):
        """
        Checks that get value returns the value in memory for a written key, and an error for a key that is not
        written, also when it has a default value.

        By the agreement of the feature team (eclipse-score/reference_integration#383), get value does not fall back
        to the default value; the default value is read with get default value. The current text of
        comp_req__kvs__value_get_api says otherwise and is to be corrected (eclipse-score/reference_integration#485).

        Steps:
        1. Run steps 1 to 5 of "test_key_exists".
        2. Compare the logged results of get value with the keys written in memory.

        Expected result:
        - The program ends without an error.
        - Written keys: the last value written in memory.
          - "stored": 30 as loaded, and 31 after "stored_overwritten", so the value in memory wins over the value 30
            in the storage files.
          - After "set": "with_default" = 11 and "without_default" = 21. After "set_to_default_value":
            "with_default" = 10.
          - After "overwritten": "with_default" = 12 and "without_default" = 22, so the second set replaced the first.
        - A key that is not written: an error, no value, also when it has a default value.
          - "with_default" in "never_written", "removed", "reset", "stored_overwritten" and "stored_removed".
          - "without_default" in every state except "set" and "overwritten".
          - "stored" after "reset" and "stored_removed".
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        written = KEY_STATES[state].written
        logged = {
            log.key: "Err" if log.value.startswith("Err(") else log.value for log in logs_info_level.get_logs("key")
        }
        # Written keys: the written value. Keys that are not written: an error.
        expected = {key: f"Ok({written[key]})" if key in written else "Err" for key in KEYS}
        assert logged == expected

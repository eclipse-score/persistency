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
import re
import shlex
from dataclasses import dataclass
from pathlib import Path
from typing import Any
from zlib import adler32

import pytest
from common import (
    CommonScenario,
    ResultCode,
    execute_checked,
    read_file_on_target,
    run_named_scenario,
    stored_values,
)
from score.itf.plugins.core import Target
from test_properties import add_test_properties
from testing_utils import LogContainer, ScenarioResult

pytestmark = pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")

INSTANCE_ID = 1

# Current KVS file and its hash file, written by flush.
STORAGE = [f"kvs_{INSTANCE_ID}_0.hash", f"kvs_{INSTANCE_ID}_0.json"]

# Steps of the 'explicit_flush' scenario.
STEPS = [
    "open",
    # Changes before the first flush, on empty storage: set.
    "set",
    "set",
    "set",
    "flush",
    # Changes to stored keys, then the second flush: overwrite, remove.
    "set",
    "remove",
    "flush",
    # Changes after the last flush, never flushed: overwrite, remove, reset, set, restore of snapshot 1.
    "set",
    "remove",
    "reset",
    "set",
    "restore",
    "drop",
]

# Key-value pairs in memory at the last flush.
FLUSHED_VALUES = {"kept": 1, "overwritten": 2}

# Number of flushes of the 'flush_value_types' scenario: more flushes than the 3 snapshots kept by default, so every
# snapshot slot is written.
FLUSH_COUNT = 4

# Key-value pairs that the 'flush_value_types' scenario sets before the first flush, one per value type, in the KVS
# file format (type-tagged values). The string value and the last key hold characters that JSON must escape.
TYPED_VALUES = {
    "u8": {"t": "u8", "v": 123},
    "u16": {"t": "u16", "v": 12345},
    "u32": {"t": "u32", "v": 1234},
    "u64": {"t": "u64", "v": 123456789012},
    "i8": {"t": "i8", "v": -123},
    "i16": {"t": "i16", "v": -12345},
    "i32": {"t": "i32", "v": -321},
    "i64": {"t": "i64", "v": -123456789012},
    "f32": {"t": "f32", "v": 12.5},
    "f64": {"t": "f64", "v": -5432.1},
    "bool": {"t": "bool", "v": True},
    "null": {"t": "null", "v": None},
    "str": {"t": "str", "v": 'quote " backslash \\ slash / newline \n tab \t control \x01 umlaut ä'},
    "arr": {
        "t": "arr",
        "v": [
            {"t": "i32", "v": 1},
            {"t": "str", "v": ""},
            {"t": "bool", "v": False},
            {"t": "null", "v": None},
            {"t": "arr", "v": []},
            {"t": "obj", "v": {}},
        ],
    },
    "obj": {
        "t": "obj",
        "v": {
            "nested": {"t": "f64", "v": 1.5},
            "deeper": {"t": "obj", "v": {"text": {"t": "str", "v": "value"}}},
        },
    },
    'quote" backslash\\': {"t": "i32", "v": 7},
}

# Data file of the current KVS storage and of snapshot 1.
CURRENT_DATA_FILE = f"kvs_{INSTANCE_ID}_0.json"
SNAPSHOT_DATA_FILE = f"kvs_{INSTANCE_ID}_1.json"


def kvs_parameters(temp_dir: Path, **extra: Any) -> dict[str, Any]:
    return {"kvs_parameters": {"instance_id": INSTANCE_ID, "dir": str(temp_dir), **extra}}


def parse_storage(storage: str) -> dict[str, str]:
    """
    Fingerprint of each file in the working directory by file name, as logged by the 'explicit_flush' scenario.
    """
    return dict(entry.split("=") for entry in storage.split(",")) if storage else {}


@dataclass(frozen=True)
class AfterExit:
    """
    Scenario run, and the process started after it has exited that loads the KVS from storage.
    """

    result: ScenarioResult
    restart: ScenarioResult


@add_test_properties(
    fully_verifies=["comp_req__kvs__explicit_flush"],
    test_type="requirements-based",
    derivation_technique="requirements-analysis",
)
class TestExplicitFlush(CommonScenario):
    """
    Verifies that changes in memory are written to storage only by an explicit flush, never automatically.

    A new KVS is changed, flushed twice, changed again without a flush, and dropped. The storage files change
    only at a flush, and a new process then loads exactly the data of the second flush.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.persistency.explicit_flush"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return kvs_parameters(temp_dir)

    @pytest.fixture(scope="class")
    def after_exit(
        self,
        target: Target,
        target_path: Path,
        temp_dir: Path,
        execution_timeout: float,
        results: ScenarioResult,
    ) -> AfterExit:
        """
        Run the scenario, then load the KVS, which must exist, in a new process that logs every key-value pair.
        """
        config = kvs_parameters(temp_dir, kvs_load="required")
        restart = run_named_scenario(target, target_path, "cit.persistency.stored_values", config, execution_timeout)
        return AfterExit(results, restart)

    def test_storage_written_only_at_flush(self, results: ScenarioResult, logs_info_level: LogContainer):
        """
        Checks that the storage files change only when flush is called.

        After each of steps 2 to 13, the scenario program logs the name of every file in the working folder
        and a fingerprint of the file (a hash of its content).

        Steps:
        1. Create an empty working folder on the target and start the scenario program.
        2. Open the KVS (key-value store) with this folder as its storage.
        3. Set three keys: "kept" = 1, "overwritten" = 1, "removed" = 3.
        4. Flush (write the data in memory to the storage files).
        5. Set "overwritten" to 2.
        6. Remove the key "removed".
        7. Flush again.
        8. Set "overwritten" to 20.
        9. Remove the key "kept".
        10. Reset (remove all keys from memory).
        11. Set "added" to 4.
        12. Restore snapshot 1 (replace the data in memory with the copy from the first flush, which the KVS keeps).
        13. Drop the KVS object (close it without a flush).
        14. Check the files logged after each step: after a flush, that the data file and its hash file exist;
            after any other step, that they are the same as after the step before.

        Expected result:
        - The program ends without an error and logs steps 2 to 13 in this order.
        - After steps 2 and 3, the working folder is empty.
        - After each flush (steps 4 and 7), the data file "kvs_1_0.json" and its hash file "kvs_1_0.hash" exist.
        - After every other step, including after the drop, the file names and fingerprints are the same as after
          the step before.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        steps = [(log.step, parse_storage(log.storage)) for log in logs_info_level.get_logs("step")]
        assert [step for step, _ in steps] == STEPS

        # The working directory is empty before the KVS is opened.
        previous: dict[str, str] = {}
        for index, (step, storage) in enumerate(steps):
            if step == "flush":
                assert set(STORAGE) <= storage.keys(), f"KVS or hash file missing after the flush at step {index}"
            else:
                assert storage == previous, f"storage written without flush, at step {index} '{step}'"
            previous = storage

    def test_flushed_state_loaded_after_restart(self, after_exit: AfterExit, target_path: Path):
        """
        Checks that a new process loads exactly the data of the last flush.

        Steps:
        1. Run the scenario program of "test_storage_written_only_at_flush" (steps 1 to 13 there) until it ends.
        2. Start a second program on the target, as a new process.
        3. Open the same KVS. The open fails if the storage files do not exist.
        4. Read all keys with their values and log them.
        5. Compare the logged keys and values with the data at the second flush.

        Expected result:
        - Both programs end without an error.
        - The second program loads exactly "kept" = 1 and "overwritten" = 2, the data at the second flush.
          So the changes made after the second flush (steps 8 to 12 there) were not saved, not even when
          the KVS was dropped or the first program ended.
        """
        assert after_exit.result.return_code == ResultCode.SUCCESS, after_exit.result.stderr
        assert after_exit.restart.return_code == ResultCode.SUCCESS, after_exit.restart.stderr
        assert stored_values(after_exit.restart, target_path) == FLUSHED_VALUES


@add_test_properties(
    fully_verifies=["comp_req__kvs__pers_data_csum", "comp_req__kvs__pers_data_store_fmt"],
    partially_verifies=["comp_req__kvs__value_serialize"],
    test_type="interface-test",
    derivation_technique="equivalence-classes",
)
class TestStorageFilesFormatAndChecksum(CommonScenario):
    """
    Verifies the storage files that flush writes: every data file, the current one and each snapshot, is JSON text
    that holds the flushed key-value pairs, and has a hash file next to it that holds the checksum of exactly this
    data file. The flushed data has a value of every value type, and a string value and a key with characters that
    JSON must escape. The default-values file is not covered: the component reads it, but does not write it. For
    comp_req__kvs__value_serialize, only the serialization of every value type to JSON is covered, not the
    deserialization.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.persistency.flush_value_types"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return {**kvs_parameters(temp_dir), "flush_count": FLUSH_COUNT}

    def test_data_files_are_json(self, target: Target, temp_dir: Path, results: ScenarioResult):
        """
        Checks that every data file is JSON text that holds the key-value pairs of a flush.

        Steps:
        1. Create an empty working folder on the target and start the scenario program.
        2. Open the KVS (key-value store) with this folder as its storage, with the default of 3 snapshots.
        3. Set one key per value type: "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64", "bool",
           "null", "str", "arr" and "obj". The "f32" value is exactly representable, so both implementations write
           the same number.
           The string holds a quote, a backslash, a slash, a newline, a tab, the control character 0x01 and the
           non-ASCII letter "ä". The array holds a value of several types, an empty string, an empty array and an
           empty object. The object holds a nested object. Also set the key `quote" backslash\\` to an I32 value.
        4. Set "generation" to 1 and flush (write the data in memory to the storage files). Repeat with "generation"
           = 2, 3 and 4. At each flush, the data of the flush before becomes snapshot 1 ("kvs_1_1.json"), snapshot 1
           becomes snapshot 2, and so on.
        5. List the working folder and download every data file "kvs_1_<n>.json" from the target.
        6. Decode each data file as UTF-8 and parse it as JSON text.

        Expected result:
        - The program ends without an error.
        - At least the current data file "kvs_1_0.json" and snapshot 1 "kvs_1_1.json" exist. The number of older
          snapshots is not checked.
        - Each data file is UTF-8 text that parses as JSON. A parser error, for example for a character that is not
          escaped, fails the test.
        - "kvs_1_0.json" holds the values of step 3 and "generation" = 4 in the KVS file format: the object
          {"t": "obj", "v": {...}} with every key and its type-tagged value {"t": <type>, "v": <value>}.
        - Every snapshot "kvs_1_<n>.json" holds the same with "generation" = 4 - n, the data of the n-th flush before
          the last one.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        names = execute_checked(target, f"ls {shlex.quote(str(temp_dir))}").split()
        data_files = [name for name in names if re.fullmatch(rf"kvs_{INSTANCE_ID}_\d+\.json", name)]
        assert {CURRENT_DATA_FILE, SNAPSHOT_DATA_FILE} <= set(data_files), f"files: {names}"

        # KVS file content of each flush, by the value of "generation".
        flushed = {
            generation: {"t": "obj", "v": {**TYPED_VALUES, "generation": {"t": "i32", "v": generation}}}
            for generation in range(1, FLUSH_COUNT + 1)
        }
        current = json.loads(read_file_on_target(target, temp_dir / CURRENT_DATA_FILE).decode("utf-8"))
        assert current == flushed[FLUSH_COUNT]
        for name in sorted(set(data_files) - {CURRENT_DATA_FILE}):
            snapshot = int(name.removeprefix(f"kvs_{INSTANCE_ID}_").removesuffix(".json"))
            content = json.loads(read_file_on_target(target, temp_dir / name).decode("utf-8"))
            assert content == flushed.get(FLUSH_COUNT - snapshot), name

    def test_hash_files_hold_checksum(self, target: Target, temp_dir: Path, results: ScenarioResult):
        """
        Checks that every data file has a hash file next to it that holds the checksum of exactly this data file.

        Steps:
        1. Run the scenario program of "test_data_files_are_json" (steps 1 to 4 there).
        2. List the working folder.
        3. For every data file "kvs_1_<n>.json", download it and its hash file "kvs_1_<n>.hash" from the target.
        4. Compute the Adler-32 checksum of the data file on the test host, as 4 bytes big-endian. This is the
           checksum that the KVS checks when it loads a data file.

        Expected result:
        - The program ends without an error.
        - At least "kvs_1_0.json" and "kvs_1_1.json" exist.
        - For every data file, the hash file with the same number "kvs_1_<n>.hash" exists in the same folder.
        - Each hash file holds exactly the 4 bytes of step 4, so its checksum was generated from the content of its
          data file.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        names = execute_checked(target, f"ls {shlex.quote(str(temp_dir))}").split()
        data_files = [name for name in names if re.fullmatch(rf"kvs_{INSTANCE_ID}_\d+\.json", name)]
        assert {CURRENT_DATA_FILE, SNAPSHOT_DATA_FILE} <= set(data_files), f"files: {names}"

        for name in data_files:
            hash_name = name.removesuffix(".json") + ".hash"
            assert hash_name in names, f"no hash file next to {name}, files: {names}"
            data = read_file_on_target(target, temp_dir / name)
            checksum = adler32(data).to_bytes(length=4, byteorder="big")
            assert read_file_on_target(target, temp_dir / hash_name) == checksum, name

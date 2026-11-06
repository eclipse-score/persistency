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
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import pytest
from common import CommonScenario, ResultCode, logs_from_stdout, run_named_scenario
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


def kvs_parameters(temp_dir: Path, **extra: Any) -> dict[str, Any]:
    return {"kvs_parameters": {"instance_id": INSTANCE_ID, "dir": str(temp_dir), **extra}}


def parse_storage(storage: str) -> dict[str, str]:
    """
    Fingerprint of each file in the working directory by file name, as logged by the 'explicit_flush' scenario.
    """
    return dict(entry.split("=") for entry in storage.split(",")) if storage else {}


def stored_values(result: ScenarioResult, target_path: Path) -> dict[str, int]:
    """
    Key-value pairs logged by the 'stored_values' scenario.
    """
    logs = logs_from_stdout(result.stdout)
    logs = logs.get_logs(field="target", pattern=f"{target_path.name}.*").get_logs(field="level", value="INFO")
    return {log.key: log.value for log in logs.get_logs("key")}


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

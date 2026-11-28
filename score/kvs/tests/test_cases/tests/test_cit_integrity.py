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
from pathlib import Path
from typing import Any
from zlib import adler32

import pytest
from common import (
    CommonScenario,
    ResultCode,
    create_defaults_file,
    create_defaults_json,
    run_named_scenario,
    stored_values,
    temp_dir_common,
    write_file_on_target,
)
from score.itf.plugins.core import Target
from test_properties import add_test_properties
from testing_utils import LogContainer, ScenarioResult

INSTANCE_ID = 1

# Data file and hash file of the current KVS storage (snapshot 0) and of snapshot 1.
DATA_FILE = f"kvs_{INSTANCE_ID}_0.json"
HASH_FILE = f"kvs_{INSTANCE_ID}_0.hash"
SNAPSHOT_DATA_FILE = f"kvs_{INSTANCE_ID}_1.json"
SNAPSHOT_HASH_FILE = f"kvs_{INSTANCE_ID}_1.hash"

# Key-value pairs of the intact data file.
STORED_VALUES = {"stored": 1}
# Intact data file: the key "stored" with the I32 value 1, in the KVS file format (type-tagged values).
STORED_DATA = json.dumps({"t": "obj", "v": {"stored": {"t": "i32", "v": 1}}}).encode()
# Hash file of the intact data file: Adler-32 checksum, 4 bytes big-endian.
STORED_HASH = adler32(STORED_DATA).to_bytes(length=4, byteorder="big")
# Data file with "stored" = 2. Together with STORED_HASH, it is a data file changed after its hash file was written.
MODIFIED_DATA = json.dumps({"t": "obj", "v": {"stored": {"t": "i32", "v": 2}}}).encode()
# Hash file of MODIFIED_DATA.
MODIFIED_HASH = adler32(MODIFIED_DATA).to_bytes(length=4, byteorder="big")

# Key-value pairs of intact snapshots by slot: slot 1 is the data of the flush before the current storage, slot 2 the
# data of the flush before that, and so on. They differ from each other and from the data files of STORAGE_FILES.
SNAPSHOT_VALUES = {1: {"stored": 10, "added": 10}, 2: {"stored": 20}, 3: {"stored": 30}}

# Key read by the scenario "cit.default_values.default_values", and its value in the default-values file.
DEFAULT_KEY = "test_number"
DEFAULT_VALUE = 111.1

# Storage files written before the KVS is opened, by storage state: file name and content.
STORAGE_FILES: dict[str, dict[str, bytes]] = {
    # No storage.
    "missing": {},
    # Intact storage.
    "valid": {DATA_FILE: STORED_DATA, HASH_FILE: STORED_HASH},
    # Data file changed after the hash was written; still valid JSON, only the checksum detects it.
    "modified": {DATA_FILE: MODIFIED_DATA, HASH_FILE: STORED_HASH},
    # Data file cut in half, as by an interrupted write.
    "truncated": {DATA_FILE: STORED_DATA[: len(STORED_DATA) // 2], HASH_FILE: STORED_HASH},
    # Hash file cut to 2 of its 4 bytes, as by an interrupted write.
    "hash_truncated": {DATA_FILE: STORED_DATA, HASH_FILE: STORED_HASH[:2]},
    # Data file without its hash file.
    "hash_missing": {DATA_FILE: STORED_DATA},
    # Hash file without its data file.
    "data_missing": {HASH_FILE: STORED_HASH},
}

# Open result by KVS load mode and storage state, as comp_req__kvs__open_need_kvs_behavior requires:
# "error": the open reports an error; "empty": the KVS opens with no keys; "loaded": the KVS opens with STORED_VALUES.
# "valid" with "required" and "optional" checks that intact files written by the test are loaded.
# Not listed, because the requirement does not define it: "ignored" with "valid".
OPEN_CASES = [
    # Required: an error whenever the storage is missing or fails the integrity check.
    ("required", "valid", "loaded"),
    ("required", "missing", "error"),
    ("required", "data_missing", "error"),
    ("required", "modified", "error"),
    ("required", "truncated", "error"),
    ("required", "hash_truncated", "error"),
    ("required", "hash_missing", "error"),
    # Optional: empty storage when no storage file exists, an error when it exists but fails the integrity check.
    ("optional", "valid", "loaded"),
    ("optional", "missing", "empty"),
    # Only a hash file: no storage file exists.
    ("optional", "data_missing", "empty"),
    ("optional", "modified", "error"),
    ("optional", "truncated", "error"),
    ("optional", "hash_truncated", "error"),
    ("optional", "hash_missing", "error"),
    # Ignored: empty storage in every case.
    ("ignored", "missing", "empty"),
    ("ignored", "data_missing", "empty"),
    ("ignored", "modified", "empty"),
    ("ignored", "truncated", "empty"),
    ("ignored", "hash_truncated", "empty"),
    ("ignored", "hash_missing", "empty"),
]

# Error of an "error" case by storage state, as printed after "Failed to create KVS instance: " by the scenario program:
# Rust prints the ErrorCode name, C++ the message of the ErrorCode (score/kvs/error.cpp). The error shows where the open
# failed:
# - No data file: the data file cannot be read. C++ reports kKvsFileReadError, Rust FileNotFound.
# - Data file changed or cut, or hash file cut: the integrity check fails (ValidationFailed): the checksum does not
#   match, or the hash file is too short.
# - No hash file: the hash file cannot be read (KvsHashFileReadError), so the integrity check cannot run.
OPEN_ERRORS: dict[str, dict[str, str]] = {
    "missing": {"cpp": "KVS file read error", "rust": "FileNotFound"},
    "data_missing": {"cpp": "KVS file read error", "rust": "FileNotFound"},
    "modified": {"cpp": "Validation failed", "rust": "ValidationFailed"},
    "truncated": {"cpp": "Validation failed", "rust": "ValidationFailed"},
    "hash_truncated": {"cpp": "Validation failed", "rust": "ValidationFailed"},
    "hash_missing": {"cpp": "KVS hash file read error", "rust": "KvsHashFileReadError"},
}


@add_test_properties(
    fully_verifies=["comp_req__kvs__open_need_kvs_behavior", "comp_req__kvs__corruption_detect_open"],
    partially_verifies=["comp_req__kvs__pers_data_csum_vrfy"],
    test_type="fault-injection",
    derivation_technique="equivalence-classes",
)
@pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")
@pytest.mark.parametrize(("kvs_load", "storage", "expected"), OPEN_CASES, scope="class")
class TestOpenByKvsLoadMode(CommonScenario):
    """
    Verifies that opening a KVS whose storage is missing, interrupted or corrupted follows the KVS load mode:
    Required reports an error, Optional reports an error unless no storage file exists, Ignored opens an empty KVS.
    Data of a storage that fails the integrity check is never loaded.
    """

    @pytest.fixture(scope="class")
    def temp_dir(self, target: Target, version: str, kvs_load: str, storage: str) -> Generator[Path, None, None]:
        # One working directory per case, named after it: each case writes its own storage files.
        yield from temp_dir_common(target, self.__class__.__name__, version, kvs_load, storage)

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.persistency.stored_values"

    @pytest.fixture(scope="class")
    def test_config(self, target: Target, temp_dir: Path, kvs_load: str, storage: str) -> dict[str, Any]:
        """
        Write the storage files of the storage state, return the input that opens the KVS in the KVS load mode.
        """
        for name, content in STORAGE_FILES[storage].items():
            write_file_on_target(target, temp_dir / name, content)
        return {"kvs_parameters": {"instance_id": INSTANCE_ID, "dir": str(temp_dir), "kvs_load": kvs_load}}

    def test_open(
        self,
        request: pytest.FixtureRequest,
        version: str,
        kvs_load: str,
        storage: str,
        results: ScenarioResult,
        logs_info_level: LogContainer,
        expected: str,
    ):
        """
        Checks that the open reports an error or opens the KVS with the expected keys, as the load mode requires.

        The storage is the data file "kvs_1_0.json" and its hash file "kvs_1_0.hash". The hash file holds the
        Adler-32 checksum (a 4-byte hash) of the data file; the integrity check compares the two.

        Steps:
        1. Create an empty working folder on the target.
        2. Write the files of the storage state under test into it:
           - "missing": no file;
           - "valid": a data file with the key "stored" = 1, and its hash file;
           - "modified": a data file with "stored" = 2, and the hash file of the data file with "stored" = 1;
           - "truncated": the first half of the "valid" data file, and the hash file of the whole data file;
           - "hash_truncated": the "valid" data file, and the first 2 of the 4 bytes of its hash file;
           - "hash_missing": the "valid" data file only;
           - "data_missing": the hash file of the "valid" data file only.
        3. Start the scenario program. It opens the KVS (key-value store) in this folder with the load mode under
           test: "required", "optional" or "ignored".
        4. If the open succeeds, the program logs every key with its value and ends. If the open fails, the
           program stops with the message "Failed to create KVS instance" and the error.

        Expected result:
        - "error" cases ("required" with every state except "valid"; "optional" with "modified", "truncated",
          "hash_truncated" and "hash_missing"): the program stops with "Failed to create KVS instance" and the error
          of the storage state, which shows where the open failed:
          - "missing" and "data_missing": the data file cannot be read. C++ "KVS file read error", Rust
            "FileNotFound".
          - "modified", "truncated" and "hash_truncated": the integrity check fails, because the checksum does not
            match or the hash file is too short. C++ "Validation failed", Rust "ValidationFailed". For "truncated",
            whose data is not valid JSON, this also shows that the checksum is checked before the JSON is parsed; a
            parser error would mean it was not.
          - "hash_missing": the hash file cannot be read, so the integrity check cannot run. C++ "KVS hash file read
            error", Rust "KvsHashFileReadError".
        - "empty" cases ("optional" with "missing" and "data_missing", where no data file exists; "ignored" with
          every state except "valid"): the program ends without an error and logs no key, so no data of the storage
          files is loaded.
        - "loaded" cases ("required" and "optional" with "valid"): the program ends without an error and logs
          exactly "stored" = 1, so intact storage files written by the test are loaded.

        Known defect (eclipse-score/persistency#770): Rust treats a data file without its hash file as "no storage".
        Rust "hash_missing" cases are strict xfail: "optional" at the open check, "required" at the error check.
        """
        logged = {log.key: log.value for log in logs_info_level.get_logs("key")}
        if expected == "error":
            if (kvs_load, storage, version) == ("optional", "hash_missing", "rust"):
                request.applymarker(
                    pytest.mark.xfail(
                        strict=True,
                        reason="Rust Optional opens an empty KVS for an existing data file without a hash file, while "
                        "comp_req__kvs__open_need_kvs_behavior requires an error (eclipse-score/persistency#770)",
                    )
                )
            assert results.return_code == ResultCode.PANIC, f"expected the open to fail, logged values: {logged}"
            if (storage, version) == ("hash_missing", "rust"):
                request.applymarker(
                    pytest.mark.xfail(
                        strict=True,
                        reason="Rust reports FileNotFound, the code for a missing data file, instead of "
                        "KvsHashFileReadError for a missing hash file (eclipse-score/persistency#770)",
                    )
                )
            assert f"Failed to create KVS instance: {OPEN_ERRORS[storage][version]}" in results.stderr
        else:
            assert results.return_code == ResultCode.SUCCESS, results.stderr
            assert logged == (STORED_VALUES if expected == "loaded" else {})


@add_test_properties(
    partially_verifies=["comp_req__kvs__pers_data_csum_vrfy", "comp_req__kvs__snapshot_restore_api"],
    test_type="fault-injection",
    derivation_technique="requirements-analysis",
)
@pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")
@pytest.mark.parametrize("snapshot_hash", ["matching", "mismatching"], scope="class")
class TestSnapshotRestoreVerifiesChecksum(CommonScenario):
    """
    Verifies that a snapshot restore checks the checksum of the snapshot: with a matching checksum it loads the
    snapshot data, with a mismatching checksum it reports an error and does not load the snapshot data.
    """

    @pytest.fixture(scope="class")
    def temp_dir(self, target: Target, version: str, snapshot_hash: str) -> Generator[Path, None, None]:
        # One working directory per case, named after it: each case writes its own snapshot hash file.
        yield from temp_dir_common(target, self.__class__.__name__, version, snapshot_hash)

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.persistency.values_after_restore"

    @pytest.fixture(scope="class")
    def test_config(self, target: Target, temp_dir: Path, snapshot_hash: str) -> dict[str, Any]:
        """
        Write an intact current storage and a snapshot 1 with "stored" = 2, whose hash file matches the snapshot data
        file or is the hash file of the current data file.
        """
        files = {
            DATA_FILE: STORED_DATA,
            HASH_FILE: STORED_HASH,
            SNAPSHOT_DATA_FILE: MODIFIED_DATA,
            SNAPSHOT_HASH_FILE: MODIFIED_HASH if snapshot_hash == "matching" else STORED_HASH,
        }
        for name, content in files.items():
            write_file_on_target(target, temp_dir / name, content)
        return {"kvs_parameters": {"instance_id": INSTANCE_ID, "dir": str(temp_dir), "kvs_load": "required"}}

    def test_restore(self, results: ScenarioResult, logs_info_level: LogContainer, snapshot_hash: str):
        """
        Checks that a snapshot restore loads the snapshot data only if the snapshot matches its checksum.

        Steps:
        1. Create an empty working folder on the target.
        2. Write the current storage: a data file "kvs_1_0.json" with the key "stored" = 1, and its hash file
           "kvs_1_0.hash" (the Adler-32 checksum of the data file).
        3. Write snapshot 1 (a previous state the KVS keeps): a data file "kvs_1_1.json" with "stored" = 2, and a
           hash file "kvs_1_1.hash" that depends on the case:
           - "matching": the hash file of "kvs_1_1.json", so the checksum matches;
           - "mismatching": the hash file of the data file with "stored" = 1, so the checksum does not match.
        4. Start the scenario program. It opens the KVS (key-value store) in this folder; the open fails if the
           current storage cannot be loaded.
        5. Restore snapshot 1 (replace the data in memory with the data of snapshot 1). If the restore fails, log
           its error.
        6. Log every key in memory with its value.

        Expected result:
        - The program ends without an error.
        - "matching": the restore reports no error, and the logged values are exactly "stored" = 2, the data of
          snapshot 1.
        - "mismatching": the restore reports an error, and the key "stored" does not have the value 2 of snapshot 1,
          so no data of snapshot 1 is loaded.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        load_error = logs_info_level.find_log("load_error")
        logged = {log.key: log.value for log in logs_info_level.get_logs("key")}
        if snapshot_hash == "matching":
            assert load_error is None, f"snapshot restore failed: {load_error}"
            assert logged == {"stored": 2}
        else:
            assert load_error is not None, "snapshot restore did not report an error"
            assert logged.get("stored") != 2, "data of snapshot 1 was loaded"


@add_test_properties(
    partially_verifies=["comp_req__kvs__pers_data_csum_vrfy", "comp_req__kvs__discard_pending_changes_api"],
    test_type="fault-injection",
    derivation_technique="requirements-analysis",
)
@pytest.mark.parametrize(
    "version",
    [
        "cpp",
        pytest.param(
            "rust",
            marks=pytest.mark.skip(
                reason="Rust KVS has no API to reload from storage (https://github.com/eclipse-score/persistency/issues/713)"
            ),
        ),
    ],
    scope="class",
)
@pytest.mark.parametrize("storage_hash", ["matching", "mismatching"], scope="class")
class TestDiscardPendingChangesVerifiesChecksum(CommonScenario):
    """
    Verifies that discarding pending changes, which reloads the data from storage, checks the checksum: with a
    matching checksum it loads the stored data, with a mismatching checksum it reports an error and does not load it.
    """

    @pytest.fixture(scope="class")
    def temp_dir(self, target: Target, version: str, storage_hash: str) -> Generator[Path, None, None]:
        # One working directory per case, named after it: each case writes its own hash file.
        yield from temp_dir_common(target, self.__class__.__name__, version, storage_hash)

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.persistency.values_after_discard"

    @pytest.fixture(scope="class")
    def test_config(self, target: Target, temp_dir: Path, storage_hash: str) -> dict[str, Any]:
        """
        Write a storage with "stored" = 2, whose hash file matches the data file or is the hash file of the data file
        with "stored" = 1, and return the input that opens the KVS without loading it.
        """
        files = {DATA_FILE: MODIFIED_DATA, HASH_FILE: MODIFIED_HASH if storage_hash == "matching" else STORED_HASH}
        for name, content in files.items():
            write_file_on_target(target, temp_dir / name, content)
        return {"kvs_parameters": {"instance_id": INSTANCE_ID, "dir": str(temp_dir), "kvs_load": "ignored"}}

    def test_discard(self, results: ScenarioResult, logs_info_level: LogContainer, storage_hash: str):
        """
        Checks that reloading the stored data loads it only if the data file matches its checksum.

        Steps:
        1. Create an empty working folder on the target.
        2. Write the storage: a data file "kvs_1_0.json" with the key "stored" = 2, and a hash file "kvs_1_0.hash"
           (the Adler-32 checksum) that depends on the case:
           - "matching": the hash file of "kvs_1_0.json", so the checksum matches;
           - "mismatching": the hash file of the data file with "stored" = 1, so the checksum does not match.
        3. Start the scenario program. It opens the KVS (key-value store) in this folder with the load mode
           "ignored", so the open does not read the storage and memory is empty.
        4. Discard pending changes (reload the data from storage, dropping changes in memory). If this fails, log
           its error.
        5. Log every key in memory with its value.

        Expected result:
        - The program ends without an error.
        - "matching": discarding pending changes reports no error, and the logged values are exactly "stored" = 2,
          the stored data.
        - "mismatching": discarding pending changes reports an error, and the key "stored" does not have the value 2
          of the storage, so no data of the storage is loaded.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        load_error = logs_info_level.find_log("load_error")
        logged = {log.key: log.value for log in logs_info_level.get_logs("key")}
        if storage_hash == "matching":
            assert load_error is None, f"discard pending changes failed: {load_error}"
            assert logged == {"stored": 2}
        else:
            assert load_error is not None, "discard pending changes did not report an error"
            assert logged.get("stored") != 2, "data of the storage was loaded"


@add_test_properties(
    partially_verifies=[
        "comp_req__kvs__pers_data_csum_vrfy",
        "comp_req__kvs__default_value_cfg",
        "comp_req__kvs__default_val_chksum",
    ],
    test_type="fault-injection",
    derivation_technique="requirements-analysis",
)
@pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")
@pytest.mark.parametrize("defaults_hash", ["matching", "mismatching"], scope="class")
class TestOpenDefaultsVerifiesChecksum(CommonScenario):
    """
    Verifies that opening a KVS checks the checksum of its default-values file: with a matching checksum the default
    values are available, with a mismatching checksum the open reports an error.
    """

    @pytest.fixture(scope="class")
    def temp_dir(self, target: Target, version: str, defaults_hash: str) -> Generator[Path, None, None]:
        # One working directory per case, named after it: each case writes its own default-values file.
        yield from temp_dir_common(target, self.__class__.__name__, version, defaults_hash)

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.default_values.default_values"

    @pytest.fixture(scope="class")
    def test_config(self, target: Target, temp_dir: Path, defaults_hash: str) -> dict[str, Any]:
        """
        Write a default-values file and its hash file, for "mismatching" change the default-values file afterwards.
        Return the input that opens the KVS with optional default values.
        """
        defaults_path = create_defaults_file(target, temp_dir, INSTANCE_ID, {DEFAULT_KEY: ("f64", DEFAULT_VALUE)})
        if defaults_hash == "mismatching":
            write_file_on_target(target, defaults_path, create_defaults_json({DEFAULT_KEY: ("f64", 222.2)}).encode())
        return {"kvs_parameters": {"instance_id": INSTANCE_ID, "dir": str(temp_dir), "defaults": "optional"}}

    def test_open(self, results: ScenarioResult, logs_info_level: LogContainer, defaults_hash: str):
        """
        Checks that the open provides the default values only if the default-values file matches its checksum.

        Steps:
        1. Create an empty working folder on the target.
        2. Write the default-values file "kvs_1_default.json" with the default value "test_number" = 111.1, and its
           hash file "kvs_1_default.hash" (the Adler-32 checksum of the default-values file).
        3. "mismatching" only: overwrite the default-values file with "test_number" = 222.2, so the checksum no
           longer matches.
        4. Start the scenario program. It opens the KVS (key-value store) in this folder with optional default
           values, which loads the default-values file if it exists. If the open fails, the program stops with the
           message "Failed to create KVS instance" and the error.
        5. If the open succeeds, the program logs the default value of "test_number" (and then changes the value,
           flushes and opens the KVS again, which this test does not check).

        Expected result:
        - "matching": the program ends without an error, and the first logged default value of "test_number" is
          111.1, the value of the default-values file.
        - "mismatching": the program stops with "Failed to create KVS instance", so the open reports an error and
          provides no default value of the changed file.
        """
        logged = logs_info_level.get_logs("key", value=DEFAULT_KEY)
        if defaults_hash == "matching":
            assert results.return_code == ResultCode.SUCCESS, results.stderr
            assert logged[0].default_value == f"Ok(F64({DEFAULT_VALUE}))"
        else:
            defaults = [log.default_value for log in logged]
            assert results.return_code == ResultCode.PANIC, f"expected the open to fail, default values: {defaults}"
            assert "Failed to create KVS instance" in results.stderr


@add_test_properties(
    partially_verifies=[
        "comp_req__kvs__recovery_snapshot_restore",
        "comp_req__kvs__corruption_detect_open",
        "comp_req__kvs__open_need_kvs_behavior",
        "comp_req__kvs__pers_data_csum_vrfy",
        "comp_req__kvs__snapshot_restore_api",
        "comp_req__kvs__explicit_flush",
    ],
    test_type="fault-injection",
    derivation_technique="requirements-analysis",
)
@pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")
@pytest.mark.parametrize("storage", ["modified", "truncated", "hash_truncated", "hash_missing"], scope="class")
@pytest.mark.parametrize("snapshot", list(SNAPSHOT_VALUES), scope="class")
class TestRecoveryBySnapshotRestore(CommonScenario):
    """
    Verifies that after an open has detected a corrupted or interrupted current storage, the user can restore the
    snapshot of their choice, which recovers exactly the data of that snapshot, and that a flush persists the
    recovered data for the next process. Covered: a data file changed after its hash file was written, a data file or
    a hash file cut by an interrupted write, a data file without its hash file, and the snapshots in slots 1 to 3.
    For comp_req__kvs__corruption_detect_open, comp_req__kvs__open_need_kvs_behavior and
    comp_req__kvs__pers_data_csum_vrfy, only that an open with load mode Required reports the error of these storage
    states is covered; for comp_req__kvs__snapshot_restore_api, only the restore of an occupied slot selected by index;
    for comp_req__kvs__explicit_flush, only that the flush called by the user persists the restored data.
    """

    @pytest.fixture(scope="class")
    def temp_dir(self, target: Target, version: str, storage: str, snapshot: int) -> Generator[Path, None, None]:
        # One working directory per case, named after it: each case writes its own storage files.
        yield from temp_dir_common(target, self.__class__.__name__, version, storage, str(snapshot))

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.persistency.recover_from_snapshot"

    @pytest.fixture(scope="class")
    def test_config(self, target: Target, temp_dir: Path, storage: str, snapshot: int) -> dict[str, Any]:
        """
        Write the damaged current storage and the intact snapshots 1 to 3, return the input that restores the chosen
        snapshot.
        """
        files = dict(STORAGE_FILES[storage])
        for slot, values in SNAPSHOT_VALUES.items():
            data = json.dumps({"t": "obj", "v": {key: {"t": "i32", "v": value} for key, value in values.items()}})
            files[f"kvs_{INSTANCE_ID}_{slot}.json"] = data.encode()
            files[f"kvs_{INSTANCE_ID}_{slot}.hash"] = adler32(data.encode()).to_bytes(length=4, byteorder="big")
        for name, content in files.items():
            write_file_on_target(target, temp_dir / name, content)
        return {"kvs_parameters": {"instance_id": INSTANCE_ID, "dir": str(temp_dir)}, "snapshot_id": snapshot}

    def test_open_detects_damage(
        self,
        request: pytest.FixtureRequest,
        version: str,
        storage: str,
        results: ScenarioResult,
        logs_info_level: LogContainer,
    ):
        """
        Checks that an open with load mode Required reports the damaged current storage, before the recovery.

        Steps:
        1. Create an empty working folder on the target.
        2. Write the current storage in the state under test:
           - "modified" (corrupted data): a data file "kvs_1_0.json" with the key "stored" = 2, and the hash file
             "kvs_1_0.hash" (the Adler-32 checksum) of the data file with "stored" = 1;
           - "truncated" (interrupted write of the data file): the first half of the data file with "stored" = 1, and
             the hash file of the whole data file;
           - "hash_truncated" (interrupted write of the hash file): the data file with "stored" = 1, and the first 2 of
             the 4 bytes of its hash file;
           - "hash_missing" (flush interrupted between writing the data file and the hash file): the data file with
             "stored" = 1, and no hash file.
        3. Write three intact snapshots (previous states that the KVS keeps), each a data file and its hash file:
           snapshot 1 "kvs_1_1.json" with "stored" = 10 and "added" = 10, snapshot 2 "kvs_1_2.json" with "stored" = 20,
           snapshot 3 "kvs_1_3.json" with "stored" = 30.
        4. Start the scenario program. It opens the KVS (key-value store) in this folder with load mode Required,
           which loads the current storage and fails if the storage is missing or fails the integrity check. If the
           open fails, the program logs the error and continues with the recovery of the other tests. If the open
           succeeds, the program logs this and ends without a recovery.

        Expected result:
        - The program ends without an error.
        - The open with load mode Required fails with the error of the storage state:
          - "modified", "truncated" and "hash_truncated": the integrity check fails, because the checksum does not
            match or the hash file is too short. C++ "Validation failed", Rust "ValidationFailed".
          - "hash_missing": the hash file cannot be read. C++ "KVS hash file read error", Rust "KvsHashFileReadError".

        Known defect (eclipse-score/persistency#770): Rust reports a missing hash file as "FileNotFound".
        The Rust "hash_missing" cases are strict xfail at the error check, after the open-fails check.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        open_error = logs_info_level.find_log("open_error")
        assert open_error is not None, "the open with load mode Required did not fail"
        if (storage, version) == ("hash_missing", "rust"):
            request.applymarker(
                pytest.mark.xfail(
                    strict=True,
                    reason="Rust reports FileNotFound, the code for a missing data file, instead of "
                    "KvsHashFileReadError for a missing hash file (eclipse-score/persistency#770)",
                )
            )
        assert open_error.open_error == f"Failed to create KVS instance: {OPEN_ERRORS[storage][version]}"

    def test_restore_recovers_chosen_snapshot(
        self, results: ScenarioResult, logs_info_level: LogContainer, snapshot: int
    ):
        """
        Checks that restoring the chosen snapshot recovers exactly the data of this snapshot.

        Steps:
        1. Write the storage and run the scenario program of "test_open_detects_damage" (steps 1 to 4 there).
        2. The program opens the KVS again, with load mode Ignored, which does not read the storage: memory is empty.
        3. It restores the snapshot under test, 1, 2 or 3 (replaces the data in memory with the data of this
           snapshot). If the restore fails, it logs the error.
        4. It logs every key in memory with its value.

        Expected result:
        - The program ends without an error.
        - The open with load mode Required fails, so the program runs the recovery.
        - The restore reports no error.
        - The logged values are exactly the data of the chosen snapshot: "stored" = 10 and "added" = 10 for snapshot
          1, "stored" = 20 for snapshot 2, "stored" = 30 for snapshot 3. So no data of the damaged current storage or
          of another snapshot is in memory.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        assert logs_info_level.find_log("required_open") is None, (
            "the open with load mode Required did not fail, so the program ran no recovery"
        )
        load_error = logs_info_level.find_log("load_error")
        assert load_error is None, f"snapshot restore failed: {load_error.load_error}"
        assert {log.key: log.value for log in logs_info_level.get_logs("key")} == SNAPSHOT_VALUES[snapshot]

    def test_recovered_data_loaded_after_flush(
        self,
        request: pytest.FixtureRequest,
        target: Target,
        target_path: Path,
        temp_dir: Path,
        execution_timeout: float,
        version: str,
        storage: str,
        results: ScenarioResult,
        logs_info_level: LogContainer,
        snapshot: int,
    ):
        """
        Checks that a flush after the restore persists the recovered data, so that a new process loads it.

        Steps:
        1. Write the storage and run the scenario program of "test_restore_recovers_chosen_snapshot" (steps 1 to 4
           there).
        2. After the restore, the program flushes (writes the data in memory to the storage files) and logs the error
           if the flush fails. Then it ends.
        3. Start a second program on the target, as a new process. It opens the same KVS with load mode Required,
           which fails if the current storage is missing or fails the integrity check, and logs every key with its
           value.

        Expected result:
        - The first program ends without an error, and its open with load mode Required fails, so it runs the
          recovery.
        - The flush of the first program reports no error.
        - The second program ends without an error, so the current storage written by the flush is intact.
        - The second program loads exactly the data of the chosen snapshot.

        Known defect: in the Rust KVS, the snapshot rotation of the flush rejects a current data file without its hash
        file, so the flush fails with "IntegrityCorrupted" and the recovered data cannot be persisted
        (eclipse-score/persistency#729). The Rust "hash_missing" cases are marked as expected failures (strict xfail)
        at the check of the flush error, after the check that the program ends without an error. They turn into test
        failures once they pass.
        """
        assert results.return_code == ResultCode.SUCCESS, results.stderr
        assert logs_info_level.find_log("required_open") is None, (
            "the open with load mode Required did not fail, so the program ran no recovery"
        )
        flush_error = logs_info_level.find_log("flush_error")
        if (storage, version) == ("hash_missing", "rust"):
            request.applymarker(
                pytest.mark.xfail(
                    strict=True,
                    reason="Rust flush fails with IntegrityCorrupted: snapshot rotation rejects a current data file "
                    "without its hash file, so the recovered data cannot be persisted (eclipse-score/persistency#729)",
                )
            )
        assert flush_error is None, f"flush failed: {flush_error.flush_error}"

        config = {"kvs_parameters": {"instance_id": INSTANCE_ID, "dir": str(temp_dir), "kvs_load": "required"}}
        restart = run_named_scenario(target, target_path, "cit.persistency.stored_values", config, execution_timeout)
        assert restart.return_code == ResultCode.SUCCESS, restart.stderr
        assert stored_values(restart, target_path) == SNAPSHOT_VALUES[snapshot]

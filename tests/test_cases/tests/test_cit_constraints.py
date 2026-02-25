# Copyright (c) 2025 Qorix
#
# Test cases for KVS constraint configuration

"""Test cases for constraints configuration (compile-time and runtime)"""

from pathlib import Path
from typing import Any

import pytest
from .common import CommonScenario, ResultCode
from testing_utils import LogContainer, ScenarioResult
from test_properties import add_test_properties


@add_test_properties(
    fully_verifies=[
        "comp_req__persistency__constraints_v2",
        "comp_req__persistency__snapshot_max_num_v2",
    ],
    test_type="requirements-based",
    derivation_technique="interface-test",
)
@pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")
@pytest.mark.parametrize(
    "constraint_type,constraint_value",
    [
        pytest.param("runtime", 5, id="runtime_snapshot_max_5"),
        pytest.param("runtime", 10, id="runtime_snapshot_max_10"),
        pytest.param("compile_time", 3, id="compile_time_max_snapshots"),
    ],
    scope="class",
)
class TestConstraintConfiguration(CommonScenario):
    """Tests for compile-time and runtime constraint configuration

    Requirements: The component shall allow configuration of KVS constraints
    at compile-time using source code constants or at runtime using a
    configuration file.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.constraints.ConstraintConfiguration"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path, constraint_type: str, constraint_value: int) -> dict[str, Any]:
        return {
            "kvs_parameters": {
                "instance_id": 1,
                "dir": str(temp_dir),
                "snapshot_max_count": constraint_value if constraint_type == "runtime" else 10,
            },
            "constraint_type": constraint_type,
            "constraint_value": constraint_value,
        }

    def test_constraint_configuration(
        self,
        test_config: dict[str, Any],
        results: ScenarioResult,
        logs_info_level: LogContainer,
        constraint_type: str,
        constraint_value: int,
        version: str,
    ):
        """Test that constraints can be configured at compile-time and runtime

        - Runtime constraints: snapshot_max_count via configuration file
        - Compile-time constraints: KVS_MAX_SNAPSHOTS constant in source code
        """
        assert results.return_code == ResultCode.SUCCESS

        if constraint_type == "runtime":
            # Runtime constraint should be configurable via parameter
            # Note: C++ runtime values are capped by compile-time KVS_MAX_SNAPSHOTS (=3)
            #       Rust has no such compile-time limit
            log_configured = logs_info_level.find_log("configured_max")
            assert log_configured is not None, "configured_max log not found"
            configured_max = int(log_configured.configured_max)

            if version == "cpp":
                # C++ runtime config is capped at compile-time maximum (3)
                expected_max = min(constraint_value, 3)  # KVS_MAX_SNAPSHOTS = 3
            else:  # rust
                # Rust accepts the runtime config value without compile-time capping
                expected_max = constraint_value

            assert configured_max == expected_max, \
                f"Runtime constraint not properly configured: expected {expected_max}, got {configured_max}"

            log_applied = logs_info_level.find_log("constraint_applied")
            assert log_applied is not None, "constraint_applied log not found"
            # Handle both integer and boolean values from logs
            constraint_applied_value = log_applied.constraint_applied
            if isinstance(constraint_applied_value, bool):
                constraint_applied = 1 if constraint_applied_value else 0
            else:
                constraint_applied = int(constraint_applied_value)
            # Applied if configured_max matches expected (capped at compile-time max for C++)
            assert constraint_applied == 1, "Runtime constraint not applied"

        elif constraint_type == "compile_time":
            # Compile-time constraint should be hardcoded
            log_compile_max = logs_info_level.find_log("compile_time_max")
            assert log_compile_max is not None, "compile_time_max log not found"
            compile_time_max = int(log_compile_max.compile_time_max)
            assert compile_time_max == constraint_value, \
                f"Compile-time KVS_MAX_SNAPSHOTS should be {constraint_value}, got {compile_time_max}"

            log_exists = logs_info_level.find_log("compile_time_constraint_exists")
            assert log_exists is not None, "compile_time_constraint_exists log not found"
            # Handle both integer and boolean values
            constraint_exists_value = log_exists.compile_time_constraint_exists
            if isinstance(constraint_exists_value, bool):
                compile_time_exists = 1 if constraint_exists_value else 0
            else:
                compile_time_exists = int(constraint_exists_value)
            assert compile_time_exists == 1, "Compile-time constraint not found"


@add_test_properties(
    fully_verifies=[
        "comp_req__persistency__permission_control_v2",
        "comp_req__persistency__permission_err_hndl_v2"
    ],
    test_type="requirements-based",
    derivation_technique="error-test",
)
@pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")
class TestPermissionControl(CommonScenario):
    """Tests for filesystem permission control

    Requirement: The component shall rely on the underlying filesystem for
    access and permission management and shall not implement its own access
    or permission controls.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.constraints.PermissionControl"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return {
            "kvs_parameters": {
                "instance_id": 1,
                "dir": str(temp_dir),
                "snapshot_max_count": 10,
            },
        }

    def test_filesystem_permissions(
        self,
        test_config: dict[str, Any],
        results: ScenarioResult,
        logs_info_level: LogContainer,
    ):
        """Test that KVS relies on filesystem permissions

        Verify that KVS uses filesystem for permission management and does not
        implement its own permission layer.
        """
        assert results.return_code == ResultCode.SUCCESS

        # Verify that KVS attempts filesystem operations without custom permission layer
        log_uses_fs = logs_info_level.find_log("uses_filesystem")
        assert log_uses_fs is not None, "uses_filesystem log not found"
        uses_filesystem = int(log_uses_fs.uses_filesystem)
        assert uses_filesystem == 1, "KVS should use filesystem for storage"

        log_custom = logs_info_level.find_log("custom_permission_layer")
        assert log_custom is not None, "custom_permission_layer log not found"
        custom_permission_layer = int(log_custom.custom_permission_layer)
        assert custom_permission_layer == 0, "KVS should not implement custom permission controls"


@add_test_properties(
    fully_verifies=["comp_req__persistency__permission_err_hndl_v2"],
    test_type="requirements-based",
    derivation_technique="error-test",
)
@pytest.mark.parametrize("version", ["cpp", "rust"], scope="class")
@pytest.mark.parametrize(
    "error_type",
    [
        pytest.param("read_denied", id="read_permission_denied"),
        pytest.param("write_denied", id="write_permission_denied"),
    ],
    scope="class",
)
class TestPermissionErrorHandling(CommonScenario):
    """Tests for permission error handling

    Requirement: The component shall report any access or permission errors
    encountered at the filesystem level to the application.
    """

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.constraints.PermissionErrorHandling"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path, error_type: str) -> dict[str, Any]:
        return {
            "kvs_parameters": {
                "instance_id": 1,
                "dir": str(temp_dir),
                "snapshot_max_count": 10,
            },
            "error_type": error_type,
        }

    def test_permission_error_reporting(
        self,
        test_config: dict[str, Any],
        results: ScenarioResult,
        logs_info_level: LogContainer,
        error_type: str,
    ):
        """Test that filesystem permission errors are properly reported

        Verify that:
        - Read permission errors are reported to application
        - Write permission errors are reported to application
        - Errors include appropriate error information

        Note: This test may not work correctly when running as root,
        since root can bypass filesystem permissions.
        """
        import os

        # Skip test if running as root (UID 0) since root bypasses permissions
        if os.getuid() == 0:
            pytest.skip("Permission tests cannot run as root (root bypasses filesystem permissions)")

        # Note: The scenario may exit with error code (non-SUCCESS)
        # when permission denied errors occur, which is expected behavior

        # Verify error was detected and reported
        log_detected = logs_info_level.find_log("error_detected")
        assert log_detected is not None, f"error_detected log not found for {error_type}"
        error_detected = int(log_detected.error_detected)
        assert error_detected == 1, \
            f"Permission error should be detected for {error_type}"

        log_reported = logs_info_level.find_log("error_reported")
        assert log_reported is not None, f"error_reported log not found for {error_type}"
        error_reported = int(log_reported.error_reported)
        assert error_reported == 1, \
            f"Permission error should be reported to application for {error_type}"

        # Check that error message contains useful information
        log_msg = logs_info_level.find_log("error_message")
        assert log_msg is not None, f"error_message log not found for {error_type}"
        error_msg = str(log_msg.error_message).lower()
        assert len(error_msg) > 0, "Error message should not be empty"
        # Check for various error indicators (flexible matching)
        has_error_indicator = any(keyword in error_msg for keyword in [
            "permission", "access", "denied", "error", "fail", "cannot", "unable"
        ])
        assert has_error_indicator, \
            f"Error message should indicate permission/access issue, got: {error_msg}"

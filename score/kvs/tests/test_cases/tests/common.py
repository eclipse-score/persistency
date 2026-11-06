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
import logging
import shlex
import tempfile
import uuid
from collections.abc import Generator
from pathlib import Path

import pytest
from score.itf.plugins.core import Target
from testing_utils import BuildTools, LogContainer, Scenario, ScenarioResult

logger = logging.getLogger(__name__)

# Directory on the target for files created by the tests.
TARGET_TMP_DIR = Path("/tmp")


class ResultCode:
    """
    Test scenario exit codes.
    """

    SUCCESS = 0
    PANIC = 101
    SIGKILL = -9
    SIGABRT = -6


def execute_checked(target: Target, command: str) -> str:
    exit_code, output = target.execute(command)
    assert exit_code == 0, f"Command `{command}` failed with exit code {exit_code}: {output.decode()}"
    return output.decode()


def _test_path(target: Target, operator: str, path: Path) -> bool:
    """
    Evaluate 'test <operator> <path>' on the target, fail if the check itself doesn't run.

    Parameters
    ----------
    target : Target
        Target to check path on.
    operator : str
        'test' operator, e.g., '-e'.
    path : Path
        Path on the target.
    """
    command = f"test {operator} {shlex.quote(str(path))}"
    exit_code, output = target.execute(command)
    # 'test' exits with 0 (true), 1 (false) or above 1 (error).
    assert exit_code in (0, 1), f"Command `{command}` failed with exit code {exit_code}: {output.decode()}"
    return exit_code == 0


def path_exists(target: Target, path: Path) -> bool:
    return _test_path(target, "-e", path)


def file_exists(target: Target, path: Path) -> bool:
    return _test_path(target, "-f", path)


def write_file_on_target(target: Target, path: Path, content: bytes) -> None:
    with tempfile.TemporaryDirectory() as local_dir:
        local_path = Path(local_dir) / path.name
        local_path.write_bytes(content)
        target.upload(str(local_path), str(path))


def temp_dir_common(target: Target, base_name: str, *args: str) -> Generator[Path, None, None]:
    """
    Create temporary directory on the target and remove it after test.
    Common implementation to be reused by fixtures.

    Returns generator providing unique path to temporary directory.
    E.g., '/tmp/<BASE_NAME>-<ARG1>-<ARG2>-<UNIQUE_ID>/'.

    Parameters
    ----------
    target : Target
        Target to create directory on.
    base_name : str
        Base directory name.
        'self.__class__.__name__' use is recommended.
    *args : Any
        Other parameters to be included in directory name.
    """
    parts = [base_name, *args, uuid.uuid4().hex[:8]]
    dir_path = TARGET_TMP_DIR / "-".join(parts)
    execute_checked(target, f"mkdir -p {shlex.quote(str(dir_path))}")
    logger.info("Created temporary directory: %s", dir_path)
    yield dir_path
    execute_checked(target, f"rm -rf {shlex.quote(str(dir_path))}")
    logger.info("Removed temporary directory: %s", dir_path)


class CommonScenario(Scenario):
    @pytest.fixture(scope="class")
    def build_tools(self) -> BuildTools:
        # Required by 'Scenario', unused: Bazel installs test scenarios on the target.
        raise NotImplementedError

    @pytest.fixture(scope="class")
    def target_path(self, target: Target, version: str, request: pytest.FixtureRequest) -> Path:
        """
        Path to test scenario executable on the target.

        Parameters
        ----------
        target : Target
            Target with test scenario executable installed.
        version : str
            Test scenario implementation, "cpp" or "rust".
        request : FixtureRequest
            Test request built-in fixture.
        """
        option = f"--{version}-target-path"
        target_path = request.config.getoption(option)
        if target_path is None:
            raise pytest.UsageError(f'"{option}" must be set')
        if not _test_path(target, "-x", target_path):
            raise pytest.UsageError(f'"{option}": no executable at {target_path} on the target')
        return target_path

    @pytest.fixture(scope="class")
    def results(self, target: Target, command: list[str], execution_timeout: float) -> ScenarioResult:
        """
        Execute test scenario executable on the target and return results.

        Parameters
        ----------
        target : Target
            Target to execute test scenario on.
        command : list[str]
            Command to invoke.
        execution_timeout : float
            Test execution timeout in seconds.
        """
        # Stderr is redirected to a file to keep it apart from stdout.
        stderr_path = TARGET_TMP_DIR / f"scenario-{uuid.uuid4().hex}.stderr"
        # A hanging scenario is killed with SIGKILL, it might ignore SIGTERM.
        target_command = (
            f"cd {TARGET_TMP_DIR} && RUST_BACKTRACE=1 timeout -s KILL {execution_timeout} {shlex.join(command)} "
            f"2>{stderr_path}"
        )
        logger.info("Running command: `%s`", target_command)
        exit_code, stdout = target.execute(target_command)
        stderr = execute_checked(target, f"cat {stderr_path} && rm -f {stderr_path}")
        logger.debug("Command finished with exit code %d, stderr:\n%s", exit_code, stderr)

        # Shell reports a process killed by signal N as 128 + N.
        return_code = 128 - exit_code if exit_code > 128 else exit_code
        hang = return_code == ResultCode.SIGKILL
        if hang:
            logger.warning("Command timed out after %ss", execution_timeout)
        return ScenarioResult(stdout.decode(), stderr, return_code, hang)

    @pytest.fixture(scope="class")
    def temp_dir(self, target: Target, version: str) -> Generator[Path, None, None]:
        yield from temp_dir_common(target, self.__class__.__name__, version)

    @pytest.fixture(scope="class")
    def logs_target(self, target_path: Path, logs: LogContainer) -> LogContainer:
        return logs.get_logs(field="target", pattern=f"{target_path.name}.*")

    @pytest.fixture(scope="class")
    def logs_info_level(self, logs_target: LogContainer) -> LogContainer:
        return logs_target.get_logs(field="level", value="INFO")

    @pytest.fixture(autouse=True)
    def print_to_report(
        self,
        request: pytest.FixtureRequest,
        logs: LogContainer,
        logs_target: LogContainer,
    ) -> None:
        """
        Print traces to stdout.

        Allowed "--traces" values:
        - "none" - show no traces.
        - "target" - show traces generated by test code.
        - "all" - show all traces.

        Parameters
        ----------
        request : FixtureRequest
            Test request built-in fixture.
        logs : LogContainer
            Test scenario execution logs.
        logs_target : LogContainer
            Logs with messages generated strictly by the tested code.
        """
        traces_param = request.config.getoption("--traces")
        match traces_param:
            case "all":
                traces = logs
            case "target":
                traces = logs_target
            case "none":
                traces = LogContainer()
            case _:
                raise RuntimeError(f'Invalid "--traces" value: {traces_param}')

        for trace in traces:
            print(trace)  # noqa: T201  Traces are shown through pytest output capture.

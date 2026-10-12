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

"""
Test helpers.
"""

from collections.abc import Generator
from pathlib import Path

import pytest


@pytest.fixture(scope="package")
def target_tmp_dir(request) -> Path:
    """
    Target temp directory, as provided by the param.
    """
    return Path(request.config.getoption("--target-tmp-dir"))


def text(output) -> str:
    """
    Decode target output, which the ssh backends return as bytes and the docker one as str.
    """
    return output.decode(errors="replace") if isinstance(output, bytes) else str(output)


def execute(target, command: str) -> str:
    """
    Execute command on target, expect return code 0.
    """
    exit_code, output = target.execute(f"{command} 2>&1")
    assert exit_code == 0, f"{command!r} failed on the target: {text(output)}"
    return text(output)


@pytest.fixture(scope="package")
def deployed_package(request, target, target_tmp_dir: Path) -> Path:
    """
    Upload the package to the target, unpack it, and return its root directory.
    """
    root_dir = target_tmp_dir / "kvs_tool_tests"

    # On-host archive path.
    deploy_path = Path(request.config.getoption("--deploy"))
    # On-target archive path.
    archive_path = root_dir / deploy_path.name

    # Remove previous, upload, unpack.
    execute(target, f"rm -rf {root_dir} && mkdir {root_dir}")
    target.upload(deploy_path, archive_path)
    execute(target, f"tar -xf {archive_path} -C {root_dir} && rm -f {archive_path}")
    return root_dir


@pytest.fixture
def data_dir(target, target_tmp_dir: Path) -> Generator[Path, None, None]:
    """
    Create a `test_data` directory on enter, remove on exit.
    """
    test_data_dir = target_tmp_dir / "test_data"
    execute(target, f"mkdir -p {test_data_dir}")
    yield test_data_dir
    execute(target, f"rm -r {test_data_dir}")

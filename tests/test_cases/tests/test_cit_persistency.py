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
from testing_utils import LogContainer, ScenarioResult
from test_properties import add_test_properties

pytestmark = pytest.mark.parametrize("version", ["rust"], scope="class")


@add_test_properties(
    fully_verifies=["comp_req__persistency__persist_data_com_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestExplicitFlush(CommonScenario):
    """Verifies that disabling flush on exit but manually flushing ensures data is persisted correctly."""

    NUM_VALUES = 5

    @pytest.fixture(scope="class")
    def scenario_name(self) -> str:
        return "cit.persistency.explicit_flush"

    @pytest.fixture(scope="class")
    def test_config(self, temp_dir: Path) -> dict[str, Any]:
        return {
            "kvs_parameters": {
                "instance_id": 2,
                "dir": str(temp_dir),
            }
        }

    def test_data_stored(self, results: ScenarioResult, logs_info_level: LogContainer):
        assert results.return_code == ResultCode.SUCCESS

        for i in range(self.NUM_VALUES):
            log = logs_info_level.find_log("key", value=f"test_number_{i}")
            assert log is not None
            assert log.value == f"Ok(F64({12.3 * i}))"


# Note: The following tests verify requirements but need extended scenarios
# They are marked as TODO until scenario implementations are added

@pytest.mark.skip(reason="Requires scenario implementation - comp_req__persistency__pers_data_csum_v2")
@add_test_properties(
    fully_verifies=["comp_req__persistency__pers_data_csum_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestDataChecksumGeneration:
    """TODO: Verifies checksum file generation - needs scenario support"""
    pass


@pytest.mark.skip(reason="Requires scenario implementation - comp_req__persistency__pers_data_csum_vrfy_v2")
@add_test_properties(
    fully_verifies=["comp_req__persistency__pers_data_csum_vrfy_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestDataChecksumVerification:
    """TODO: Verifies checksum verification - needs scenario support"""
    pass


@pytest.mark.skip(reason="Requires scenario implementation - comp_req__persistency__value_serialize_v2")
@add_test_properties(
    fully_verifies=["comp_req__persistency__value_serialize_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestValueSerialization:
    """TODO: Verifies JSON serialization - needs scenario support"""
    pass


@pytest.mark.skip(reason="Requires scenario implementation - comp_req__persistency__pers_data_store_fmt_v2")
@add_test_properties(
    fully_verifies=["comp_req__persistency__pers_data_store_fmt_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestDataStorageFormat:
    """TODO: Verifies JSON storage format - needs scenario support"""
    pass


@pytest.mark.skip(reason="Requires scenario implementation - comp_req__persistency__pers_data_store_bnd_v2")
@add_test_properties(
    fully_verifies=["comp_req__persistency__pers_data_store_bnd_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestFileAPIUsage:
    """TODO: Verifies file API usage - needs scenario support"""
    pass


@pytest.mark.skip(reason="Requires scenario implementation - comp_req__persistency__pers_data_schema_v2")
@add_test_properties(
    fully_verifies=["comp_req__persistency__pers_data_schema_v2"],
    test_type="requirements-based",
    derivation_technique="requirements-based",
)
class TestJSONSchemaFlexibility:
    """TODO: Verifies JSON schema flexibility - needs scenario support"""
    pass

"""
Smoke test for Rust-C++ tests.
"""

from typing import Any
import pytest
from testing_utils import (
    BazelTools,
    BuildTools,
    LogContainer,
    ScenarioResult,
)
from common_scenario import CommonScenario


class TestBasic(CommonScenario):
    @pytest.fixture(scope="class")
    def scenario_name(self, *_, **__) -> str:
        return "basic.basic"

    @pytest.fixture(scope="class")
    def test_config(self, *_, **__) -> dict[str, Any]:
        return {"kvs_parameters": {"instance_id": 2, "flush_on_exit": False}}

    def test_returncode_ok(self, results: ScenarioResult):
        assert results.return_code == 0

    def test_trace_ok(self, logs_target: LogContainer):
        lc = logs_target.get_logs_by_field("example_key", value="example_value")
        assert len(lc) == 1


class TestBasicCpp(TestBasic):
    @pytest.fixture(scope="class")
    def build_tools(self, *_, **__) -> BuildTools:
        return BazelTools(option_prefix="cpp")


class TestBasicRust(TestBasic):
    @pytest.fixture(scope="class")
    def build_tools(self, *_, **__) -> BuildTools:
        return BazelTools(option_prefix="rust")

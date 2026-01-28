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

load("@rules_rust//rust:defs.bzl", "rust_clippy")
load("@score_docs_as_code//:docs.bzl", "docs")
load("@score_tooling//:defs.bzl", "cli_helper", "copyright_checker", "dash_license_checker", "setup_starpls", "use_format_targets")
load("//:project_config.bzl", "PROJECT_CONFIG")
load("@score_tooling//bazel/rules/score_module:score_module.bzl", 
    "sphinx_module", 
    "architectural_design", 
    "assumptions_of_use", 
    "component_requirements",  
    "dependability_analysis", 
    "feature_requirements", 
    "safety_analysis", 
    "score_component")
    

# Creates all documentation targets:
# - `:docs` for building documentation at build-time
docs(
    data = [
        "@score_platform//:needs_json",
        "@score_process//:needs_json",
    ],
    source_dir = "docs",
)

sphinx_module(
    name = "persistency_module_doc",
    srcs = glob(
        [
            "docs/**/*.rst",
            "docs/**/*.puml",
        ],
        allow_empty = True,
    ),
    index = "docs/index.rst",
    visibility = ["//visibility:public"],
    deps = [
        "@score_process//:score_process_module",
        "@score_platform//:score_platform_module",
    ],
)

# ============================================================================
# KVS Component Artifacts
# ============================================================================

assumptions_of_use(
    name = "kvs_assumptions_of_use",
    srcs = ["docs/persistency/docs/manual/safety_manual.rst"],
)

component_requirements(
    name = "kvs_component_requirements",
    srcs = ["docs/persistency/kvs/docs/requirements/requirements.rst"],
)

architectural_design(
    name = "kvs_architectural_design",
    static = ["docs/persistency/kvs/docs/architecture/static_architecture.rst"] + glob(["docs/persistency/kvs/docs/architecture/_assets/*.puml"]),
    dynamic = ["docs/persistency/kvs/docs/architecture/dynamic_architecture.rst"],
)

safety_analysis(
    name = "kvs_safety_analysis",
    failuremodes = ["docs/persistency/kvs/docs/safety_analysis/fmea.rst"],
)

dependability_analysis(
    name = "kvs_dependability_analysis",
    safety_analysis = [":kvs_safety_analysis"],
    dfa = ["docs/persistency/kvs/docs/safety_analysis/dfa.rst"],
    fmea = ["docs/persistency/kvs/docs/safety_analysis/fmea.rst"],
    arch_design = ":kvs_architectural_design",
)

# ============================================================================
# KVS Score Component (SEooC)
# ============================================================================

score_component(
    name = "persistency_kvs",
    description = """
The Key-Value Store (KVS) component provides persistent storage capabilities 
for safety-critical applications. It offers a simple interface for storing 
and retrieving key-value pairs with support for data integrity, versioning, 
and fail-safe operations.
""",
    assumptions_of_use = [":kvs_assumptions_of_use"],
    component_requirements = [":kvs_component_requirements"],
    architectural_design = [":kvs_architectural_design"],
    dependability_analysis = [":kvs_dependability_analysis"],
    sphinx = "@score_tooling//bazel/rules/score_module:score_build",
    visibility = ["//visibility:public"],
    deps = [
        "@score_process//:score_process_module",
        "@score_platform//:score_platform_module",
    ],
)   


setup_starpls(
    name = "starpls_server",
    visibility = ["//visibility:public"],
)

copyright_checker(
    name = "copyright",
    srcs = [
        ".github",
        "docs",
        "examples",
        "src",
        "tests",
        "tools",
        "//:BUILD",
        "//:MODULE.bazel",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
)

# Needed for Dash tool to check python dependency licenses.
# This is a workaround to filter out local packages from the Cargo.lock file.
# The tool is intended for third-party content.
genrule(
    name = "filtered_cargo_lock",
    srcs = ["Cargo.lock"],
    outs = ["Cargo.lock.filtered"],
    cmd = """
    awk '
    BEGIN { skip = 0; data = "" }
    /^\\[\\[package\\]\\]/ {
        if (data != "" && !skip) print data;
        skip = 1;
        data = $$0;
        next;
    }
    data != "" { data = data "\\n" $$0 }
    # any package that has a "source = " line will not be skipped.
    /^source = / { skip = 0 }
    END { if (data != "" && !skip) print data }
    ' $(location Cargo.lock) > $@
    """,
)

dash_license_checker(
    src = ":filtered_cargo_lock",
    file_type = "",  # let it auto-detect based on project_config
    project_config = PROJECT_CONFIG,
    visibility = ["//visibility:public"],
)

cli_helper(
    name = "cli-help",
    visibility = ["//visibility:public"],
)

# Add target for formatting checks
use_format_targets()

rust_clippy(
    name = "clippy",
    testonly = True,
    tags = ["manual"],
    visibility = ["//visibility:public"],
    deps = [
        "//src/rust/rust_kvs",
        "//src/rust/rust_kvs_tool:kvs_tool",
    ],
)

alias(
    name = "kvs_cpp",
    actual = "//src/cpp/src:kvs_cpp",
    tags = ["cli_help=Build KVS CPP [build]"],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "test_kvs_cpp",
    tests = ["//src/cpp/tests:test_kvs_cpp"],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "bm_kvs_cpp",
    tests = ["//src/cpp/tests:bm_kvs_cpp"],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "unit_tests",
    tests = [
        "test_kvs_cpp",
        "//src/rust/rust_kvs:tests",
    ],
    visibility = ["//visibility:public"],
)

test_suite(
    name = "cit_tests",
    tests = [
        "//tests/test_cases:cit_cpp",
        "//tests/test_cases:cit_rust",
    ],
    visibility = ["//visibility:public"],
)

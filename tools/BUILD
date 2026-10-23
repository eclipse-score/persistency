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

# Tooling entrypoints backed by @score_tooling.
# Kept out of the root BUILD so that package stays free of these dev dependencies.
load("@score_tooling//:defs.bzl", "copyright_checker", "dash_license_checker")
load("@score_tooling//third_party/format:macros.bzl", "use_format_targets")
load("//:project_config.bzl", "PROJECT_CONFIG")

# The inner format.check test_suite is generated without a visibility.
# The root test_suite //:format.check needs to see it.
package(default_visibility = ["//:__pkg__"])

copyright_checker(
    name = "copyright",
    srcs = [
        ".github",
        ".pre-commit-config.yaml",
        "BUILD",
        "CONTRIBUTION.md",
        "MODULE.bazel",
        "REPO.bazel",
        "docs",
        "examples",
        "project_config.bzl",
        "quality",
        "score",
        "tools",
    ],
    config = "@score_tooling//cr_checker/resources:config",
    exclusion = "//:cr_checker_exclusion",
    extensions = [
        "bazel",
        "BUILD",
        "bzl",
        "c",
        "cpp",
        "h",
        "hpp",
        "ini",
        "md",
        "puml",
        "py",
        "rs",
        "rst",
        "sh",
        "yaml",
        "yml",
    ],
    template = "@score_tooling//cr_checker/resources:templates",
    visibility = ["//visibility:public"],
)

dash_license_checker(
    src = "//:filtered_cargo_lock",
    file_type = "",  # let it auto-detect based on project_config
    # The //:filtered_cargo_lock genrule only drops path crates.
    # The eclipse-score git crates (score_log*, stdout_logger, test_scenarios_rust) are first-party too.
    filter_keywords = ["github.com/eclipse-score/"],
    project_config = PROJECT_CONFIG,
    visibility = ["//visibility:public"],
)

use_format_targets(languages = [
    "python",
    "rust",
    "starlark",
    "yaml",
    "cpp",
])

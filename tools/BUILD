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

# Tooling entrypoints backed by @score_tooling and @hedron_compile_commands.
# Kept out of the root BUILD so that package stays free of these dev dependencies.
load("@hedron_compile_commands//:refresh_compile_commands.bzl", "refresh_compile_commands")

exports_files(["copyright_exclusions.txt"])

# Generate `compile_commands.json`.
# Required for `clangd` support.
refresh_compile_commands(
    name = "generate_compile_commands",
    exclude_external_sources = True,
    target_compatible_with = ["@platforms//os:linux"],
    targets = {
        "//...": "",
    },
)

# Generate `rust_project.json`.
# Required for `rust-analyzer` support.
alias(
    name = "generate_rust_project",
    actual = "@rules_rust//tools/rust_analyzer:gen_rust_project",
    target_compatible_with = ["@platforms//os:linux"],
)

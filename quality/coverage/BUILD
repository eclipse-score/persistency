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

load("@score_tooling//coverage:defs.bzl", "score_coverage_reporter", "score_coverage_scope")

# The production targets whose transitive sources define the coverage scope:
# the KVS implementations (C++ and Rust) and the KVS command line tool
# (required by feat_req__persistency__tooling, see kvs_tool.rs). Untested
# in-scope files appear in the report at exact 0% via the --empty-profile
# baseline mechanism.
#
# Keep this list in sync with the module's shipped components; once the
# module defines dependable elements, derive the scope from those instead.
score_coverage_scope(
    name = "coverage_scope",
    testonly = True,
    visibility = ["//quality/coverage:__subpackages__"],
    deps = [
        "//src/cpp/src:kvs_cpp",
        "//src/rust/rust_kvs",
        "//src/rust/rust_kvs_tool:kvs_tool",
    ],
)

# Passed to Bazel as --coverage_report_generator (see coverage.bazelrc).
# Everything else (merger, reporter, justify, effective_coverage,
# generate_coverage_html) comes from @score_tooling//coverage.
score_coverage_reporter(
    name = "reporter_wrapper",
    testonly = True,
    coverage_scope = ":coverage_scope",
    llvm_cov = "@llvm_toolchain//:llvm-cov",
    llvm_cxxfilt = "@llvm_toolchain_llvm//:bin/llvm-cxxfilt",
    llvm_profdata = "@llvm_toolchain//:llvm-profdata",
)

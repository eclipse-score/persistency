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

load("@score_tooling//api_surface:api_surface.bzl", "api_surface_test")

api_surface_test(
    name = "check",
    cc_toolchain = "@api_surface_llvm//:cc-clang-x86_64-linux",
    check_docs = False,
    lock_file = "kvs.lock.json",
    tags = ["manual"],
    target = "//score/kvs",
)

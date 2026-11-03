// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

//! Helpers shared by unit tests.

/// Create a temporary directory for a test.
/// On QNX, `/tmp` can't hold directories.
/// The test's working directory can, so the directory is created there.
pub fn tempdir() -> std::io::Result<tempfile::TempDir> {
    if cfg!(target_os = "nto") {
        tempfile::tempdir_in(".")
    } else {
        tempfile::tempdir()
    }
}

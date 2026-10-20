/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_KVS_TEST_SUPPORT_TEST_SUPPORT_H
#define SCORE_KVS_TEST_SUPPORT_TEST_SUPPORT_H

namespace score::kvs::test_support
{

/// @brief Drops the shared state of every KVS instance, as if the process had restarted.
///
/// The next `KvsBuilder::Build()` for any instance ID reads its files from disk again.
/// For tests only, the Bazel target is `testonly`.
///
/// @pre No `Kvs` handle exists. A handle that outlives the call keeps the old state,
/// so it and a newly built handle would write the same files independently.
// TODO(https://github.com/eclipse-score/persistency/issues/709): Remove once component integration tests no longer need
// it.
void ResetInstanceRegistryForTesting();

}  // namespace score::kvs::test_support

#endif  // SCORE_KVS_TEST_SUPPORT_TEST_SUPPORT_H

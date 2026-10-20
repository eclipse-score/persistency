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
#ifndef SCORE_LIB_KVS_INTERNAL_KVS_STATE_HPP
#define SCORE_LIB_KVS_INTERNAL_KVS_STATE_HPP

#include "score/kvs/kvsvalue.hpp"
#include "score/filesystem/filesystem.h"

#include <string>

namespace score::mw::per::kvs
{

/**
 * @brief Shared in-memory state for a KVS backing store.
 *
 * Handles opened for the same filename prefix share this state, including the
 * snapshot and storage limits. The storage limit applies to this store's files,
 * not to storage used by other KVS instances.
 */
struct KvsInstanceState final
{
    using KeyValueMap = std::unordered_map<std::string, KvsValue>;

    ///< Whether this state has been initialized from the backing store.
    bool initialized_{false};

    ///< Protects access to the shared state.
    std::mutex mutex_;
    ///< Values in the current snapshot.
    KeyValueMap current_values_;
    ///< Default values loaded for this store.
    KeyValueMap default_values_;
    ///< Prefix used to construct this store's file paths.
    score::filesystem::Path filename_prefix_;
    ///< Configured maximum number of snapshots for this store.
    std::size_t max_snapshots_;
    ///< Optional total storage limit for this store's files.
    std::optional<size_t> max_storage_bytes_;
};

} /* namespace score::mw::per::kvs */

#endif  // SCORE_LIB_KVS_INTERNAL_KVS_HELPER_HPP

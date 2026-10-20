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
#ifndef SCORE_LIB_KVS_INTERNAL_KVS_STATE_REGISTRY_HPP
#define SCORE_LIB_KVS_INTERNAL_KVS_STATE_REGISTRY_HPP

#include "score/kvs/error.hpp"

#include "score/kvs/internal/kvs_instance_state.hpp"
#include "score/filesystem/path.h"

namespace std
{
template<>
struct hash<score::filesystem::Path>
{
    size_t operator()(const score::filesystem::Path& v) const
    {
        return std::hash<std::string>{}(v.Native());
    }
};
}

namespace score::mw::per::kvs
{

/**
 * @brief Process-wide registry of shared KVS states.
 *
 * Each key identifies a backing store. Handles requesting the same key receive
 * the same state, while the registry mutex protects access to the key-to-state map.
 */
class KvsStateRegistry
{
    using Registry = std::unordered_map<score::filesystem::Path, std::shared_ptr<KvsInstanceState>>;

public:
    KvsStateRegistry(const KvsStateRegistry&) = delete;
    KvsStateRegistry(KvsStateRegistry&&) = delete;

    KvsStateRegistry& operator=(const KvsStateRegistry&) = delete;
    KvsStateRegistry& operator=(KvsStateRegistry&&) = delete;

    /** @brief Returns the process-wide registry instance. */
    static KvsStateRegistry& getInstance();

    /**
     * @brief Returns the state for a key, creating it when it does not exist.
     * @param key Identifier of the backing store.
     * @return Shared state associated with the key.
     */
    std::shared_ptr<KvsInstanceState> getOrCreateState(const score::filesystem::Path& key);

private:
    KvsStateRegistry() = default;

    /** Removes all key-to-state mappings; existing shared pointers remain valid. For test use only */
    void clear();

    std::mutex mutex_;  ///< Protects access to registry_.
    Registry registry_;  ///< Maps backing-store keys to their shared state.
};

} /* namespace score::mw::per::kvs */

#endif  // SCORE_LIB_KVS_INTERNAL_KVS_STATE_REGISTRY_HPP

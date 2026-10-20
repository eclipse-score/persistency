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
#include "kvs_instance_state_registry.hpp"

namespace score::mw::per::kvs
{
KvsStateRegistry& KvsStateRegistry::getInstance()
{
    static KvsStateRegistry instance;
    return instance;
}

std::shared_ptr<KvsInstanceState> KvsStateRegistry::getOrCreateState(const score::filesystem::Path& key)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto iter = registry_.find(key);
    if (iter != registry_.end())
    {
        return iter->second;
    }

    std::pair<Registry::iterator, bool> result{registry_.emplace(key, std::make_shared<KvsInstanceState>())};

    return result.second ? result.first->second : std::shared_ptr<KvsInstanceState>{};
}

void KvsStateRegistry::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    registry_.clear();
}
} /* namespace score::mw::per::kvs */

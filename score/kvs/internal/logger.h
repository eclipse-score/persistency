/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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
#ifndef SCORE_KVS_INTERNAL_LOGGER_H
#define SCORE_KVS_INTERNAL_LOGGER_H

#include "score/mw/log/logger.h"
#include <string_view>

namespace score::kvs
{

/// @brief Log context of KVS.
constexpr std::string_view kLogContext{"SKVS"};

/// @brief Returns the logger shared by all KVS instances.
inline const score::mw::log::Logger& GetLogger() noexcept
{
    static const score::mw::log::Logger instance{kLogContext};
    return instance;
}

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_LOGGER_H

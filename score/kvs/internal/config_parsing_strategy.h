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
#ifndef SCORE_KVS_INTERNAL_CONFIG_PARSING_STRATEGY_H
#define SCORE_KVS_INTERNAL_CONFIG_PARSING_STRATEGY_H

#include "score/kvs/kvs_configuration.h"
#include "score/result/result.h"

#include <cstdint>
#include <string_view>

namespace score::kvs
{

/// Version of the configuration file format.
inline constexpr std::int64_t kConfigFormatVersion{1};

/// @brief Parser of the configuration file in one file format.
class ConfigParsingStrategy
{
  public:
    ConfigParsingStrategy() noexcept = default;
    virtual ~ConfigParsingStrategy();

    /// @brief Parses a configuration file.
    /// @param path Path of the configuration file.
    /// @return The configuration, or an ErrorCode on failure:
    /// - `kConfigFileReadError` if the file cannot be opened or read.
    /// - `kInvalidConfiguration` if the content breaks the file format.
    virtual score::Result<KvsConfiguration> Parse(std::string_view path) const noexcept = 0;

  protected:
    ConfigParsingStrategy(const ConfigParsingStrategy&) = default;
    ConfigParsingStrategy(ConfigParsingStrategy&&) noexcept = default;
    ConfigParsingStrategy& operator=(const ConfigParsingStrategy&) = default;
    ConfigParsingStrategy& operator=(ConfigParsingStrategy&&) noexcept = default;
};

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_CONFIG_PARSING_STRATEGY_H

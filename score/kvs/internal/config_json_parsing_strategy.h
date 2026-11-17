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
#ifndef SCORE_KVS_INTERNAL_CONFIG_JSON_PARSING_STRATEGY_H
#define SCORE_KVS_INTERNAL_CONFIG_JSON_PARSING_STRATEGY_H

#include "score/json/json_parser.h"
#include "score/kvs/internal/config_parsing_strategy.h"
#include "score/kvs/kvs_configuration.h"
#include "score/result/result.h"

#include <array>
#include <string_view>

namespace score::kvs
{

/// Keys allowed at the root of a configuration file.
inline constexpr std::array<std::string_view, 2> kConfigRootKeys{"formatVersion", "kvsInstances"};

/// Keys allowed in an entry of `kvsInstances`.
inline constexpr std::array<std::string_view, 7> kConfigEntryKeys{"instanceId",
                                                                  "snapshotId",
                                                                  "defaults",
                                                                  "kvsLoad",
                                                                  "workingDir",
                                                                  "maxStorageBytes",
                                                                  "snapshotMaxCount"};

/// Values of the `defaults` and `kvsLoad` keys.
inline constexpr std::array<std::string_view, 3> kConfigLoadPolicyValues{"required", "optional", "ignored"};

/// @brief Parser of the configuration file in JSON.
class ConfigJsonParsingStrategy final : public ConfigParsingStrategy
{
  public:
    /// @brief Reads a JSON configuration file and parses it.
    /// @param path Path of the configuration file.
    /// @return The configuration, or an ErrorCode on failure:
    /// - `kConfigFileReadError` if the file cannot be opened or read.
    /// - `kInvalidConfiguration` if the content is not valid JSON or breaks the file format.
    score::Result<KvsConfiguration> Parse(std::string_view path) const noexcept override;

    /// @brief Converts a parsed configuration document into a configuration.
    /// @param root Root of the parsed JSON document.
    /// @return The configuration, or `kInvalidConfiguration` if the document violates the format.
    /// The whole document is rejected on the first violation.
    score::Result<KvsConfiguration> Parse(const score::json::Any& root) const noexcept;
};

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_CONFIG_JSON_PARSING_STRATEGY_H

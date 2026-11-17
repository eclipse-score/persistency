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
#ifndef SCORE_KVS_INTERNAL_CONFIG_PARSER_H
#define SCORE_KVS_INTERNAL_CONFIG_PARSER_H

#include "score/kvs/kvs_configuration.h"
#include "score/result/result.h"

#include <string_view>

namespace score::kvs
{

/// @brief Parses a configuration file with the parsing strategy of its file format.
/// JSON is the only supported file format.
/// @param path Path of the configuration file.
/// @return The configuration, or an ErrorCode on failure, see `ConfigParsingStrategy::Parse()`.
score::Result<KvsConfiguration> Parse(std::string_view path) noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_CONFIG_PARSER_H

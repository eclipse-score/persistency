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
#include "score/kvs/internal/config_parser.h"

#include "score/kvs/internal/config_json_parsing_strategy.h"

namespace score::kvs
{

score::Result<KvsConfiguration> Parse(const std::string_view path) noexcept
{
    return ConfigJsonParsingStrategy{}.Parse(path);
}

}  // namespace score::kvs

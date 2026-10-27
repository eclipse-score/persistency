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
#ifndef SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_MOCK_HPP
#define SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_MOCK_HPP

#include "score/kvs/internal/ikvs_config_parser.hpp"
#include <gmock/gmock.h>

/*
 * GoogleMock test double for `IKvsConfigParser`, used by `:unit_tests` (via the
 * `kvs_config_parser_mock` Bazel target and its `create_kvs_config_parser()` factory definition)
 * so tests can exercise `KvsBuilder::build()` without depending on real configuration-by-file
 * discovery or parsing.
 */
namespace score::mw::per::kvs
{

class KvsConfigParserMock : public IKvsConfigParser
{
  public:
    MOCK_METHOD(score::Result<std::optional<KvsConfigEntry>>, load_entry, (std::size_t instance_id), (override));
};

} /* namespace score::mw::per::kvs */

#endif /* SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_MOCK_HPP */

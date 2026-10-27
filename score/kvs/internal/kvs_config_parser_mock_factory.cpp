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
#include "score/kvs/internal/kvs_config_parser_factory.hpp"
#include "score/kvs/internal/kvs_config_parser_mock.hpp"
#include <gmock/gmock.h>

/*
 * Test-only definition of `create_kvs_config_parser()`, linked into `:unit_tests` through the
 * `kvs_config_parser_mock` Bazel target. Returns a `NiceMock<KvsConfigParserMock>` (suppresses
 * uninteresting-call warnings) whose `load_entry()` returns `std::nullopt` by default, i.e.
 * reports "no configuration file applies", so existing `KvsBuilder::build()` unit tests behave as
 * if no configuration-by-file document exists, without depending on the real `KvsConfigParser` or
 * any filesystem/environment state. Tests that need a different `load_entry()` result inject their
 * own `KvsConfigParserMock` rather than relying on the instance `KvsBuilder::build()` creates
 * internally. The production target (`kvs_config_parser_factory`) defines the same symbol for the
 * public `:kvs` target; the two factory targets are never linked into the same binary (both are
 * linked alongside `:kvs_core`, never together).
 */
namespace score::mw::per::kvs
{

std::unique_ptr<IKvsConfigParser> create_kvs_config_parser()
{
    auto mock = std::make_unique<::testing::NiceMock<KvsConfigParserMock>>();
    ON_CALL(*mock, load_entry(::testing::_))
        .WillByDefault(::testing::Return(score::Result<std::optional<KvsConfigEntry>>(std::nullopt)));
    return mock;
}

} /* namespace score::mw::per::kvs */

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
#include "score/kvs/internal/kvs_config_parser.hpp"

/*
 * Production definition of `create_kvs_config_parser()`, linked into the public `:kvs` target
 * through the `kvs_config_parser_factory` Bazel target (alongside `:kvs_core`). The mock target
 * (`kvs_config_parser_mock`) defines the same symbol for `:unit_tests` (also linked alongside
 * `:kvs_core`); the two factory targets are never linked into the same binary.
 */
namespace score::mw::per::kvs
{

std::unique_ptr<IKvsConfigParser> create_kvs_config_parser()
{
    return std::make_unique<KvsConfigParser>();
}

} /* namespace score::mw::per::kvs */

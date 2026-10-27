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
#ifndef SCORE_LIB_KVS_INTERNAL_IKVS_CONFIG_PARSER_HPP
#define SCORE_LIB_KVS_INTERNAL_IKVS_CONFIG_PARSER_HPP

#include "score/kvs/internal/kvs_config_types.hpp"
#include "score/result/result.h"
#include <cstddef>
#include <optional>

/*
 * This header declares the abstraction over deployment-time "configuration by file" discovery and
 * parsing for the Key-Value Store (KVS). It has no dependency on the Kvs class: `KvsBuilder::build()`
 * merges the entry it returns into its own settings before calling `Kvs::open()`. The interface is
 * internal (not part of the public KVS API) and exists so `KvsBuilder` can be linked against either
 * the real implementation (`KvsConfigParser`) or a test double (`KvsConfigParserMock`) without
 * depending on either concrete type; see `kvs_config_parser_factory.hpp` for the link-time choice.
 */
namespace score::mw::per::kvs
{

class IKvsConfigParser
{
  public:
    IKvsConfigParser() = default;
    virtual ~IKvsConfigParser() = default;

    IKvsConfigParser(const IKvsConfigParser&) = delete;
    IKvsConfigParser& operator=(const IKvsConfigParser&) = delete;
    IKvsConfigParser(IKvsConfigParser&&) = delete;
    IKvsConfigParser& operator=(IKvsConfigParser&&) = delete;

    /*
     * input: the `InstanceId::id` of the KVS instance about to be opened.
     * behavior: Discovers whether a configuration-by-file document applies at all, and if so
     *           parses and strictly validates it in full before looking for an entry matching
     *           `instance_id`. Performs no mutation; intended to be called once per
     *           `KvsBuilder::build()`.
     * output: - the entry whose `instance_id` matches, if one exists;
     *         - `std::nullopt` if no configuration file applies, or the file has no entry for
     *           `instance_id` (the caller then uses pure code configuration);
     *         - the error from discovery (`ErrorCode::FileNotFound`) or from parsing
     *           (`ErrorCode::KvsFileReadError`, `ErrorCode::JsonParserError`,
     *           `ErrorCode::InvalidConfiguration`). The whole file is rejected on any such error,
     *           even if the broken part belongs to a different instance.
     */
    virtual score::Result<std::optional<KvsConfigEntry>> load_entry(std::size_t instance_id) = 0;
};

} /* namespace score::mw::per::kvs */

#endif /* SCORE_LIB_KVS_INTERNAL_IKVS_CONFIG_PARSER_HPP */

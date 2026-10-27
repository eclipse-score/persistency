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
#ifndef SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_HPP
#define SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_HPP

#include "score/filesystem/filesystem.h"
#include "score/json/json_parser.h"
#include "score/kvs/error.hpp"
#include "score/kvs/internal/ikvs_config_parser.hpp"
#include "score/mw/log/logger.h"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/*
 * This header declares the production implementation of deployment-time "configuration by file"
 * discovery and parsing for the Key-Value Store (KVS). It has no dependency on the Kvs class:
 * `KvsBuilder::build()` merges the entry returned by `load_entry()` into its own settings before
 * calling `Kvs::open()`. The class is internal (not part of the public KVS API); its discovery and
 * parsing steps are exposed as public methods only so unit tests can exercise them in isolation
 * (see `kvs_config_test.cpp`), not because callers other than `load_entry()` are expected.
 */
namespace score::mw::per::kvs
{

class KvsConfigParser final : public IKvsConfigParser
{
  public:
    KvsConfigParser() = default;

    score::Result<std::optional<KvsConfigEntry>> load_entry(std::size_t instance_id) override;

    /*
     * Determine the directory containing the currently running binary.
     * Used only as part of the fallback configuration file lookup; exposed for unit testing.
     */
    std::optional<std::string> running_binary_directory() const;

    /*
     * Resolve the path of the configuration-by-file document, if any, following the discovery
     * rules:
     * 1. The `STORAGE_CONFIG_PATH` environment variable (directory or file; unset/empty is
     *    ignored). An explicitly configured target that does not exist is a hard error.
     * 2. Otherwise `<binary dir>/../etc/key_value_storage_configs.json`. A missing fallback file
     *    is not an error: pure code configuration is used in that case.
     *
     * @return On success: the resolved path, or `std::nullopt` if no configuration file applies.
     *         On failure: `ErrorCode::FileNotFound` if `STORAGE_CONFIG_PATH` names a missing
     *         target.
     */
    score::Result<std::optional<std::string>> discover_kvs_config_file_path() const;

    /*
     * Strictly parse and validate a configuration-by-file document at the given path.
     * The whole file is validated, including entries unrelated to any particular instance.
     *
     * @return On success: the list of validated entries (`instance_id` values are unique).
     *         On failure: `ErrorCode::KvsFileReadError` if the file could not be read,
     *         `ErrorCode::JsonParserError` if it is not valid JSON, or
     *         `ErrorCode::InvalidConfiguration` for any structural or semantic violation
     *         (wrong/missing `format_version`, missing/wrong-typed/unknown fields, out-of-range
     *         values, duplicate `instance_id`).
     */
    score::Result<std::vector<KvsConfigEntry>> parse_kvs_config_file(const std::string& path) const;

  private:
    /* Logs the error at ERROR level and returns it as `score::Unexpected`. */
    score::Unexpected fail(ErrorCode code, std::string_view user_message) const;

    score::Result<std::optional<KvsConfigNeedMode>> parse_need_mode(const score::json::Object& obj,
                                                                    std::string_view key) const;

    score::Result<KvsConfigEntry> parse_kvs_config_entry(const score::json::Any& elem) const;

    score::filesystem::Filesystem filesystem_{score::filesystem::FilesystemFactory{}.CreateInstance()};
    std::unique_ptr<score::mw::log::Logger> logger_{std::make_unique<score::mw::log::Logger>("SKVS")};
};

} /* namespace score::mw::per::kvs */

#endif /* SCORE_LIB_KVS_INTERNAL_KVS_CONFIG_PARSER_HPP */

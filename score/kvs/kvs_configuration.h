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
#ifndef SCORE_KVS_KVS_CONFIGURATION_H
#define SCORE_KVS_KVS_CONFIGURATION_H

#include "score/kvs/kvs.h"
#include "score/kvs/kvs_parameters.h"
#include "score/result/result.h"

#include <score/optional.hpp>

#include <array>
#include <string_view>

namespace score::kvs
{

/// @brief Options of every KVS instance of a process.
///
/// Each instance is defined once, from code with `Add()` or from a configuration file.
/// A defined instance cannot be changed or replaced.
class KvsConfiguration final
{
  public:
    /// @brief Loads the configuration from a JSON file.
    ///
    /// The JSON Schema `score/kvs/config/kvs_config.schema.json` specifies the file format.
    /// An option absent from the file takes its built-in default, never a value set in code.
    /// @param path Path of the configuration file.
    /// @return The configuration, or an ErrorCode on failure:
    /// - `kConfigFileReadError` if the file cannot be opened or read.
    /// - `kInvalidConfiguration` if the content is not valid JSON or breaks the file format.
    static score::Result<KvsConfiguration> Load(std::string_view path) noexcept;

    /// @brief Loads the configuration from `./etc/kvs_config.json`, relative to the working directory.
    /// @return The configuration, or an ErrorCode on failure, see `Load(std::string_view)`.
    static score::Result<KvsConfiguration> Load() noexcept;

    /// @brief Adds the options of one instance.
    /// @param options Options of the instance, keyed by `options.instance_id`.
    /// @return Nothing on success, or an ErrorCode on failure:
    /// - `kInvalidConfiguration` if an option is out of range.
    /// - `kInstanceAlreadyConfigured` if the instance ID is already defined.
    score::Result<void> Add(const KvsParameters& options) noexcept;

    /// @brief Returns the options of one instance.
    /// @param instance_id Identifier of the instance.
    /// @return A copy of the options, or no value if the instance is not defined.
    score::cpp::optional<KvsParameters> Find(const InstanceId& instance_id) const noexcept;

  private:
    /// Options indexed by instance ID.
    std::array<score::cpp::optional<KvsParameters>, kMaxInstanceCount> entries_{};
};

}  // namespace score::kvs

#endif  // SCORE_KVS_KVS_CONFIGURATION_H

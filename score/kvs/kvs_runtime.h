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
#ifndef SCORE_KVS_KVS_RUNTIME_H
#define SCORE_KVS_KVS_RUNTIME_H

#include "score/kvs/kvs.h"
#include "score/kvs/kvs_configuration.h"
#include "score/result/result.h"

#include <string_view>

namespace score::kvs
{

/// @brief Opens every instance of a configuration.
///
/// Loads the files of all configured instances and keeps their state for the rest of the process.
/// If one instance fails to open, no instance is opened and the call can be repeated.
/// @param configuration Options of every instance to open.
/// @return Nothing on success, or an ErrorCode on failure:
/// - `kAlreadyInitialized` if a previous call succeeded.
/// - `kKvsFileReadError` if a required defaults or KVS file cannot be read.
/// - `kKvsHashFileReadError` if the hash file of an existing defaults or KVS file cannot be read.
/// - `kValidationFailed` if a defaults or KVS file does not match its hash.
/// - `kJsonParserError` if a defaults or KVS file is not a valid JSON object.
/// - `kInvalidKey` if a defaults or KVS file contains an invalid key.
/// - `kInvalidValueType` if a defaults or KVS file contains a value of an unsupported type.
score::Result<void> Initialize(const KvsConfiguration& configuration) noexcept;

/// @brief Loads a configuration file and opens every instance it defines.
///
/// Behaves as `Initialize(const KvsConfiguration&)` with the result of `KvsConfiguration::Load(path)`.
/// If the file cannot be loaded, no instance is opened and the call can be repeated.
/// @param path Path of the configuration file.
/// @return Nothing on success, or an ErrorCode on failure:
/// - `kConfigFileReadError` if the file cannot be opened or read.
/// - `kInvalidConfiguration` if the content is not valid JSON or breaks the file format.
/// - Any error of `Initialize(const KvsConfiguration&)`.
score::Result<void> Initialize(std::string_view path) noexcept;

/// @brief Loads `./etc/kvs_config.json`, relative to the working directory, and opens every instance it defines.
///
/// Behaves as `Initialize(const KvsConfiguration&)` with the result of `KvsConfiguration::Load()`.
/// @return Nothing on success, or an ErrorCode on failure, see `Initialize(std::string_view)`.
score::Result<void> Initialize() noexcept;

/// @brief Returns a handle to an opened instance.
///
/// Reads no files.
/// All handles of one instance share its state, see `Kvs` for details.
/// @param instance_id Identifier of the instance, in the range 0..kMaxInstanceCount - 1.
/// @return A handle on success, or an ErrorCode on failure:
/// - `kInvalidInstanceId` if the instance ID is not below `kMaxInstanceCount`.
/// - `kInstanceNotInitialized` if the instance was not opened by `Initialize()`.
score::Result<Kvs> Get(const InstanceId& instance_id) noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_KVS_RUNTIME_H

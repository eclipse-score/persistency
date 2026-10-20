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
#ifndef SCORE_KVS_INTERNAL_KVS_CONSTANTS_H
#define SCORE_KVS_INTERNAL_KVS_CONSTANTS_H

#include <string_view>

namespace score::kvs
{

/// @brief File name prefix of KVS files, followed by the instance ID.
inline constexpr std::string_view kFilenamePrefix{"kvs_"};

/// @brief Separator between the instance ID and the snapshot ID in KVS file names.
inline constexpr std::string_view kSnapshotSeparator{"_"};

/// @brief File name suffix of the default value files.
inline constexpr std::string_view kDefaultSuffix{"_default"};

/// @brief File name suffix of KVS JSON files.
inline constexpr std::string_view kJsonSuffix{".json"};

/// @brief File name suffix of KVS hash files.
inline constexpr std::string_view kHashSuffix{".hash"};

/// @brief File name suffix of temporary files written before the final rename.
inline constexpr std::string_view kTempSuffix{".tmp"};

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_KVS_CONSTANTS_H

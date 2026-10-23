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
#ifndef SCORE_KVS_INTERNAL_KVS_HELPER_H
#define SCORE_KVS_INTERNAL_KVS_HELPER_H

#include "score/json/json_parser.h"
#include "score/kvs/error.h"
#include "score/kvs/kvs_value.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <string>

// This header defines helper functions used internally by the Key-Value Store (KVS) implementation.
// It exists to allow unit tests to access these internal functions.
namespace score::kvs
{

/// @brief Size of an Adler-32 checksum in bytes.
constexpr std::size_t kHashSize{4U};

/// @brief Parses an Adler-32 checksum from its big-endian byte representation.
/// @param in Stream to read the 4 checksum bytes from.
/// @return The parsed checksum.
uint32_t ParseHashAdler32(std::istream& in) noexcept;

/// @brief Calculates the Adler-32 checksum of the data.
/// @param data Data to calculate the checksum of.
/// @return The calculated checksum.
uint32_t CalculateHashAdler32(const std::string& data) noexcept;

/// @brief Splits an Adler-32 checksum into its big-endian bytes for writing.
/// @param hash The checksum to split.
/// @return The checksum bytes.
std::array<uint8_t, kHashSize> GetHashBytesAdler32(const uint32_t hash) noexcept;

/// @brief Calculates the checksum of the data and returns it as bytes for writing.
///
/// Wraps the hash algorithm, so that the algorithm can be replaced more easily.
///
/// @param data Data to calculate the checksum of.
/// @return The checksum bytes.
std::array<uint8_t, kHashSize> GetHashBytes(const std::string& data) noexcept;

/// @brief Checks if the checksum read from a stream matches the data.
///
/// Wraps the hash algorithm, so that the algorithm can be replaced more easily.
///
/// @param data_calculate Data to calculate the checksum of.
/// @param data_parse Stream to read the expected checksum from.
/// @return `true` if the checksums match.
bool CheckHash(const std::string& data_calculate, std::istream& data_parse) noexcept;

/// @brief Converts a JSON value in the KVS file format (`{"t": <type>, "v": <value>}`) to a KvsValue.
/// @param any The JSON value to convert.
/// @return The converted value on success, or ErrorCode::kInvalidValueType on failure.
score::Result<KvsValue> AnyToKvsValue(const score::json::Any& any) noexcept;

/// @brief Converts a KvsValue to a JSON value in the KVS file format (`{"t": <type>, "v": <value>}`).
/// @param kv The value to convert.
/// @return The converted JSON value on success, or ErrorCode::kInvalidValueType on failure.
score::Result<score::json::Any> KvsValueToAny(const KvsValue& kv) noexcept;

/// @brief Converts an object to a JSON value in the KVS file format (`{"t": "obj", "v": <object>}`).
///
/// Avoids wrapping the object in a KvsValue first.
/// @param object The object to convert.
/// @return The converted JSON value on success, or ErrorCode::kInvalidValueType on failure (also on a duplicate key).
score::Result<score::json::Any> KvsValueToAny(const KvsValue::Object& object) noexcept;

/// @brief Converts a map to a JSON value in the KVS file format (`{"t": "obj", "v": <object>}`).
///
/// Avoids copying the map into a KvsValue::Object first.
/// @param map The map to convert.
/// @return The converted JSON value on success, or ErrorCode::kInvalidValueType on failure.
score::Result<score::json::Any> KvsValueToAny(const KvsMap& map) noexcept;

/// @brief Checks that no object in the value has duplicate keys.
///
/// Checks nested arrays and objects as well.
/// @param value The value to check.
/// @return True if every object in the value has unique keys.
bool HasUniqueKeys(const KvsValue& value) noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_INTERNAL_KVS_HELPER_H

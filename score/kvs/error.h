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
#ifndef SCORE_KVS_ERROR_H
#define SCORE_KVS_ERROR_H

#include "score/kvs/deprecated_namespace.h"
#include "score/result/result.h"
#include <string_view>

namespace score::kvs
{

/// @brief Error codes reported by the `kvs` API.
enum class ErrorCode : score::result::ErrorCode
{
    /// Error that was not yet mapped.
    kUnmappedError = 0,

    /// File not found.
    kFileNotFound = 1,

    /// KVS file read error.
    kKvsFileReadError = 2,

    /// KVS hash file read error.
    kKvsHashFileReadError = 3,

    /// JSON parser error.
    kJsonParserError = 4,

    /// JSON generator error.
    kJsonGeneratorError = 5,

    /// Physical storage failure.
    kPhysicalStorageFailure = 6,

    /// Integrity corrupted.
    kIntegrityCorrupted = 7,

    /// Validation failed.
    kValidationFailed = 8,

    /// Encryption failed.
    kEncryptionFailed = 9,

    /// Resource is busy.
    kResourceBusy = 10,

    /// Out of storage space.
    kOutOfStorageSpace = 11,

    /// Quota exceeded.
    kQuotaExceeded = 12,

    /// Authentication failed.
    kAuthenticationFailed = 13,

    /// Key not found.
    kKeyNotFound = 14,

    /// Key default value not found.
    kKeyDefaultNotFound = 15,

    /// Serialization failed.
    kSerializationFailed = 16,

    /// Invalid snapshot ID.
    kInvalidSnapshotId = 17,

    /// Conversion failed.
    kConversionFailed = 18,

    /// Mutex failed.
    kMutexLockFailed = 19,

    /// Invalid value type.
    kInvalidValueType = 20,

    /// Key is not valid.
    kInvalidKey = 21,

    // Deprecated enumerators.
    // Each one is an alias of its k-prefixed replacement and is removed in a future release.
    UnmappedError [[deprecated("Use kUnmappedError instead. This name is removed in a future release.")]] =
        kUnmappedError,
    FileNotFound [[deprecated("Use kFileNotFound instead. This name is removed in a future release.")]] = kFileNotFound,
    KvsFileReadError [[deprecated("Use kKvsFileReadError instead. This name is removed in a future release.")]] =
        kKvsFileReadError,
    KvsHashFileReadError
        [[deprecated("Use kKvsHashFileReadError instead. This name is removed in a future release.")]] =
            kKvsHashFileReadError,
    JsonParserError [[deprecated("Use kJsonParserError instead. This name is removed in a future release.")]] =
        kJsonParserError,
    JsonGeneratorError [[deprecated("Use kJsonGeneratorError instead. This name is removed in a future release.")]] =
        kJsonGeneratorError,
    PhysicalStorageFailure
        [[deprecated("Use kPhysicalStorageFailure instead. This name is removed in a future release.")]] =
            kPhysicalStorageFailure,
    IntegrityCorrupted [[deprecated("Use kIntegrityCorrupted instead. This name is removed in a future release.")]] =
        kIntegrityCorrupted,
    ValidationFailed [[deprecated("Use kValidationFailed instead. This name is removed in a future release.")]] =
        kValidationFailed,
    EncryptionFailed [[deprecated("Use kEncryptionFailed instead. This name is removed in a future release.")]] =
        kEncryptionFailed,
    ResourceBusy [[deprecated("Use kResourceBusy instead. This name is removed in a future release.")]] = kResourceBusy,
    OutOfStorageSpace [[deprecated("Use kOutOfStorageSpace instead. This name is removed in a future release.")]] =
        kOutOfStorageSpace,
    QuotaExceeded [[deprecated("Use kQuotaExceeded instead. This name is removed in a future release.")]] =
        kQuotaExceeded,
    AuthenticationFailed
        [[deprecated("Use kAuthenticationFailed instead. This name is removed in a future release.")]] =
            kAuthenticationFailed,
    KeyNotFound [[deprecated("Use kKeyNotFound instead. This name is removed in a future release.")]] = kKeyNotFound,
    KeyDefaultNotFound [[deprecated("Use kKeyDefaultNotFound instead. This name is removed in a future release.")]] =
        kKeyDefaultNotFound,
    SerializationFailed [[deprecated("Use kSerializationFailed instead. This name is removed in a future release.")]] =
        kSerializationFailed,
    InvalidSnapshotId [[deprecated("Use kInvalidSnapshotId instead. This name is removed in a future release.")]] =
        kInvalidSnapshotId,
    ConversionFailed [[deprecated("Use kConversionFailed instead. This name is removed in a future release.")]] =
        kConversionFailed,
    MutexLockFailed [[deprecated("Use kMutexLockFailed instead. This name is removed in a future release.")]] =
        kMutexLockFailed,
    InvalidValueType [[deprecated("Use kInvalidValueType instead. This name is removed in a future release.")]] =
        kInvalidValueType,
    InvalidKey [[deprecated("Use kInvalidKey instead. This name is removed in a future release.")]] = kInvalidKey,
};

/// @brief Error domain translating `ErrorCode` codes to human-readable messages.
class KvsErrorDomain final : public score::result::ErrorDomain
{
  public:
    /// @brief Returns the human-readable message for an error code.
    /// @param code The error code to translate.
    /// @return The message for `code`.
    std::string_view MessageFor(const score::result::ErrorCode& code) const noexcept override;
};

/// @brief Creates an error in the KVS error domain.
/// @param code The error code.
/// @param user_message Optional message with additional context.
/// @return The created error.
score::result::Error MakeError(const ErrorCode code, const std::string_view user_message = "") noexcept;

}  // namespace score::kvs

#endif  // SCORE_KVS_ERROR_H

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

#include "score/result/result.h"
#include <string_view>

namespace score::mw::per::kvs
{

/// @brief Error codes reported by the `kvs` API.
enum class ErrorCode : score::result::ErrorCode
{
    /// Error that was not yet mapped.
    kUnmappedError,

    /// File not found.
    kFileNotFound,

    /// KVS file read error.
    kKvsFileReadError,

    /// KVS hash file read error.
    kKvsHashFileReadError,

    /// JSON parser error.
    kJsonParserError,

    /// JSON generator error.
    kJsonGeneratorError,

    /// Physical storage failure.
    kPhysicalStorageFailure,

    /// Integrity corrupted.
    kIntegrityCorrupted,

    /// Validation failed.
    kValidationFailed,

    /// Encryption failed.
    kEncryptionFailed,

    /// Resource is busy.
    kResourceBusy,

    /// Out of storage space.
    kOutOfStorageSpace,

    /// Quota exceeded.
    kQuotaExceeded,

    /// Authentication failed.
    kAuthenticationFailed,

    /// Key not found.
    kKeyNotFound,

    /// Key default value not found.
    kKeyDefaultNotFound,

    /// Serialization failed.
    kSerializationFailed,

    /// Invalid snapshot ID.
    kInvalidSnapshotId,

    /// Conversion failed.
    kConversionFailed,

    /// Mutex failed.
    kMutexLockFailed,

    /// Invalid value type.
    kInvalidValueType,

    /// Key is not valid.
    kInvalidKey,

    // Deprecated enumerators.
    // Each one is an alias of its k-prefixed replacement and is removed in a future release.
    UnmappedError [[deprecated("Use kUnmappedError instead. The snake_case API is removed in a future release.")]] =
        kUnmappedError,
    FileNotFound [[deprecated("Use kFileNotFound instead. The snake_case API is removed in a future release.")]] =
        kFileNotFound,
    KvsFileReadError
        [[deprecated("Use kKvsFileReadError instead. The snake_case API is removed in a future release.")]] =
            kKvsFileReadError,
    KvsHashFileReadError
        [[deprecated("Use kKvsHashFileReadError instead. The snake_case API is removed in a future release.")]] =
            kKvsHashFileReadError,
    JsonParserError [[deprecated("Use kJsonParserError instead. The snake_case API is removed in a future release.")]] =
        kJsonParserError,
    JsonGeneratorError
        [[deprecated("Use kJsonGeneratorError instead. The snake_case API is removed in a future release.")]] =
            kJsonGeneratorError,
    PhysicalStorageFailure
        [[deprecated("Use kPhysicalStorageFailure instead. The snake_case API is removed in a future release.")]] =
            kPhysicalStorageFailure,
    IntegrityCorrupted
        [[deprecated("Use kIntegrityCorrupted instead. The snake_case API is removed in a future release.")]] =
            kIntegrityCorrupted,
    ValidationFailed
        [[deprecated("Use kValidationFailed instead. The snake_case API is removed in a future release.")]] =
            kValidationFailed,
    EncryptionFailed
        [[deprecated("Use kEncryptionFailed instead. The snake_case API is removed in a future release.")]] =
            kEncryptionFailed,
    ResourceBusy [[deprecated("Use kResourceBusy instead. The snake_case API is removed in a future release.")]] =
        kResourceBusy,
    OutOfStorageSpace
        [[deprecated("Use kOutOfStorageSpace instead. The snake_case API is removed in a future release.")]] =
            kOutOfStorageSpace,
    QuotaExceeded [[deprecated("Use kQuotaExceeded instead. The snake_case API is removed in a future release.")]] =
        kQuotaExceeded,
    AuthenticationFailed
        [[deprecated("Use kAuthenticationFailed instead. The snake_case API is removed in a future release.")]] =
            kAuthenticationFailed,
    KeyNotFound [[deprecated("Use kKeyNotFound instead. The snake_case API is removed in a future release.")]] =
        kKeyNotFound,
    KeyDefaultNotFound
        [[deprecated("Use kKeyDefaultNotFound instead. The snake_case API is removed in a future release.")]] =
            kKeyDefaultNotFound,
    SerializationFailed
        [[deprecated("Use kSerializationFailed instead. The snake_case API is removed in a future release.")]] =
            kSerializationFailed,
    InvalidSnapshotId
        [[deprecated("Use kInvalidSnapshotId instead. The snake_case API is removed in a future release.")]] =
            kInvalidSnapshotId,
    ConversionFailed
        [[deprecated("Use kConversionFailed instead. The snake_case API is removed in a future release.")]] =
            kConversionFailed,
    MutexLockFailed [[deprecated("Use kMutexLockFailed instead. The snake_case API is removed in a future release.")]] =
        kMutexLockFailed,
    InvalidValueType
        [[deprecated("Use kInvalidValueType instead. The snake_case API is removed in a future release.")]] =
            kInvalidValueType,
    InvalidKey [[deprecated("Use kInvalidKey instead. The snake_case API is removed in a future release.")]] =
        kInvalidKey,
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

}  // namespace score::mw::per::kvs

#endif  // SCORE_KVS_ERROR_H

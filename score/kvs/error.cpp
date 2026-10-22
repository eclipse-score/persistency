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

#include "score/kvs/error.h"

namespace score::mw::per::kvs
{

std::string_view KvsErrorDomain::MessageFor(const score::result::ErrorCode& code) const noexcept
{
    switch (static_cast<ErrorCode>(code))
    {
        case ErrorCode::kUnmappedError:
            return "Error that was not yet mapped";
        case ErrorCode::kFileNotFound:
            return "File not found";
        case ErrorCode::kKvsFileReadError:
            return "KVS file read error";
        case ErrorCode::kKvsHashFileReadError:
            return "KVS hash file read error";
        case ErrorCode::kJsonParserError:
            return "JSON parser error";
        case ErrorCode::kJsonGeneratorError:
            return "JSON generator error";
        case ErrorCode::kPhysicalStorageFailure:
            return "Physical storage failure";
        case ErrorCode::kIntegrityCorrupted:
            return "Integrity corrupted";
        case ErrorCode::kValidationFailed:
            return "Validation failed";
        case ErrorCode::kEncryptionFailed:
            return "Encryption failed";
        case ErrorCode::kResourceBusy:
            return "Resource is busy";
        case ErrorCode::kOutOfStorageSpace:
            return "Out of storage space";
        case ErrorCode::kQuotaExceeded:
            return "Quota exceeded";
        case ErrorCode::kAuthenticationFailed:
            return "Authentication failed";
        case ErrorCode::kKeyNotFound:
            return "Key not found";
        case ErrorCode::kKeyDefaultNotFound:
            return "Key default value not found";
        case ErrorCode::kSerializationFailed:
            return "Serialization failed";
        case ErrorCode::kInvalidSnapshotId:
            return "Invalid snapshot ID";
        case ErrorCode::kConversionFailed:
            return "Conversion failed";
        case ErrorCode::kMutexLockFailed:
            return "Mutex failed";
        case ErrorCode::kInvalidValueType:
            return "Invalid value type";
        case ErrorCode::kInvalidKey:
            return "Key is not valid";
        default:
            return "Unknown Error!";
    }
}

namespace
{
constexpr KvsErrorDomain kKvsErrorDomain;
}

score::result::Error MakeError(const ErrorCode code, const std::string_view user_message) noexcept
{
    return {static_cast<score::result::ErrorCode>(code), kKvsErrorDomain, user_message};
}

}  // namespace score::mw::per::kvs

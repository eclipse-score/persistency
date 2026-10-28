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

namespace score::kvs
{

std::string_view KvsErrorDomain::MessageFor(const score::result::ErrorCode& code) const noexcept
{
    switch (static_cast<ErrorCode>(code))
    {
        case ErrorCode::UnmappedError:
            return "Error that was not yet mapped";
        case ErrorCode::FileNotFound:
            return "File not found";
        case ErrorCode::KvsFileReadError:
            return "KVS file read error";
        case ErrorCode::KvsHashFileReadError:
            return "KVS hash file read error";
        case ErrorCode::JsonParserError:
            return "JSON parser error";
        case ErrorCode::JsonGeneratorError:
            return "JSON generator error";
        case ErrorCode::PhysicalStorageFailure:
            return "Physical storage failure";
        case ErrorCode::IntegrityCorrupted:
            return "Integrity corrupted";
        case ErrorCode::ValidationFailed:
            return "Validation failed";
        case ErrorCode::EncryptionFailed:
            return "Encryption failed";
        case ErrorCode::ResourceBusy:
            return "Resource is busy";
        case ErrorCode::OutOfStorageSpace:
            return "Out of storage space";
        case ErrorCode::QuotaExceeded:
            return "Quota exceeded";
        case ErrorCode::AuthenticationFailed:
            return "Authentication failed";
        case ErrorCode::KeyNotFound:
            return "Key not found";
        case ErrorCode::KeyDefaultNotFound:
            return "Key default value not found";
        case ErrorCode::SerializationFailed:
            return "Serialization failed";
        case ErrorCode::InvalidSnapshotId:
            return "Invalid snapshot ID";
        case ErrorCode::ConversionFailed:
            return "Conversion failed";
        case ErrorCode::MutexLockFailed:
            return "Mutex failed";
        case ErrorCode::InvalidValueType:
            return "Invalid value type";
        case ErrorCode::InvalidKey:
            return "Key is not valid";
        default:
            return "Unknown Error!";
    }
}

namespace
{
constexpr KvsErrorDomain kvs_error_domain;
}

score::result::Error MakeError(const ErrorCode code, const std::string_view user_message) noexcept
{
    return {static_cast<score::result::ErrorCode>(code), kvs_error_domain, user_message};
}

}  // namespace score::kvs

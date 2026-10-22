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

#include "score/kvs/kvs_general_test.h"

namespace
{
constexpr score::kvs::KvsErrorDomain kKvsErrorDomain;
}

TEST(kvs_MessageFor, MessageFor)
{
    struct
    {
        ErrorCode code;
        std::string_view expected_message;
    } test_cases[] = {
        {ErrorCode::kUnmappedError, "Error that was not yet mapped"},
        {ErrorCode::kFileNotFound, "File not found"},
        {ErrorCode::kKvsFileReadError, "KVS file read error"},
        {ErrorCode::kKvsHashFileReadError, "KVS hash file read error"},
        {ErrorCode::kJsonParserError, "JSON parser error"},
        {ErrorCode::kJsonGeneratorError, "JSON generator error"},
        {ErrorCode::kPhysicalStorageFailure, "Physical storage failure"},
        {ErrorCode::kIntegrityCorrupted, "Integrity corrupted"},
        {ErrorCode::kValidationFailed, "Validation failed"},
        {ErrorCode::kEncryptionFailed, "Encryption failed"},
        {ErrorCode::kResourceBusy, "Resource is busy"},
        {ErrorCode::kOutOfStorageSpace, "Out of storage space"},
        {ErrorCode::kQuotaExceeded, "Quota exceeded"},
        {ErrorCode::kAuthenticationFailed, "Authentication failed"},
        {ErrorCode::kKeyNotFound, "Key not found"},
        {ErrorCode::kKeyDefaultNotFound, "Key default value not found"},
        {ErrorCode::kSerializationFailed, "Serialization failed"},
        {ErrorCode::kInvalidSnapshotId, "Invalid snapshot ID"},
        {ErrorCode::kConversionFailed, "Conversion failed"},
        {ErrorCode::kMutexLockFailed, "Mutex failed"},
        {ErrorCode::kInvalidValueType, "Invalid value type"},
        {ErrorCode::kInvalidKey, "Key is not valid"},
    };
    for (const auto& test : test_cases)
    {
        SCOPED_TRACE(static_cast<int>(test.code));
        score::result::ErrorCode code = static_cast<score::result::ErrorCode>(test.code);
        EXPECT_EQ(kKvsErrorDomain.MessageFor(code), test.expected_message);
    }

    score::result::ErrorCode invalid_code = static_cast<score::result::ErrorCode>(9999);
    EXPECT_EQ(kKvsErrorDomain.MessageFor(invalid_code), "Unknown Error!");
}

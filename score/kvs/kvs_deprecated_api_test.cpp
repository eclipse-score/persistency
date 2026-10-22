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

#include "score/kvs/kvs_general_test.h"

#include "score/quality/compiler_warnings/warnings.h"

// The old headers warn inside the header, so the suppression surrounds the includes.
// clang-format off
DISABLE_WARNING_PUSH
DISABLE_WARNING(-Wdeprecated-declarations)
// clang-format on
#include "score/kvs/error.hpp"
#include "score/kvs/kvs.hpp"
#include "score/kvs/kvsbuilder.hpp"
#include "score/kvs/kvsvalue.hpp"
DISABLE_WARNING_POP

namespace
{

// The ErrorCode values reach logs and tests through static_cast, so they must stay unchanged.
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kUnmappedError) == 0, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kFileNotFound) == 1, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kKvsFileReadError) == 2, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kKvsHashFileReadError) == 3, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kJsonParserError) == 4, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kJsonGeneratorError) == 5, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kPhysicalStorageFailure) == 6, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kIntegrityCorrupted) == 7, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kValidationFailed) == 8, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kEncryptionFailed) == 9, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kResourceBusy) == 10, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kOutOfStorageSpace) == 11, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kQuotaExceeded) == 12, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kAuthenticationFailed) == 13, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kKeyNotFound) == 14, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kKeyDefaultNotFound) == 15, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kSerializationFailed) == 16, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kInvalidSnapshotId) == 17, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kConversionFailed) == 18, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kMutexLockFailed) == 19, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kInvalidValueType) == 20, "");
static_assert(static_cast<score::result::ErrorCode>(ErrorCode::kInvalidKey) == 21, "");

// Each deprecated name has the value of its replacement.
// clang-format off
DISABLE_WARNING_PUSH
DISABLE_WARNING(-Wdeprecated-declarations)
// clang-format on
static_assert(KVS_MAX_KEY_LENGTH_BYTES == kMaxKeyLengthBytes, "");
static_assert(KVS_MAX_SNAPSHOTS_UPPER_BOUND == kMaxSnapshotsUpperBound, "");
static_assert(KVS_DEFAULT_MAX_SNAPSHOTS == kDefaultMaxSnapshots, "");
static_assert(OpenNeedDefaults::Required == OpenNeedDefaults::kRequired, "");
static_assert(OpenNeedDefaults::Optional == OpenNeedDefaults::kOptional, "");
static_assert(OpenNeedDefaults::Ignored == OpenNeedDefaults::kIgnored, "");
static_assert(OpenNeedKvs::Required == OpenNeedKvs::kRequired, "");
static_assert(OpenNeedKvs::Optional == OpenNeedKvs::kOptional, "");
static_assert(OpenNeedKvs::Ignored == OpenNeedKvs::kIgnored, "");
static_assert(OpenJsonNeedFile::Required == OpenJsonNeedFile::kRequired, "");
static_assert(OpenJsonNeedFile::Optional == OpenJsonNeedFile::kOptional, "");
static_assert(KvsValue::Type::i32 == KvsValue::Type::kI32, "");
static_assert(KvsValue::Type::u32 == KvsValue::Type::kU32, "");
static_assert(KvsValue::Type::i64 == KvsValue::Type::kI64, "");
static_assert(KvsValue::Type::u64 == KvsValue::Type::kU64, "");
static_assert(KvsValue::Type::f64 == KvsValue::Type::kF64, "");
static_assert(KvsValue::Type::Boolean == KvsValue::Type::kBoolean, "");
static_assert(KvsValue::Type::String == KvsValue::Type::kString, "");
static_assert(KvsValue::Type::Null == KvsValue::Type::kNull, "");
static_assert(KvsValue::Type::Array == KvsValue::Type::kArray, "");
static_assert(KvsValue::Type::Object == KvsValue::Type::kObject, "");
static_assert(ErrorCode::UnmappedError == ErrorCode::kUnmappedError, "");
static_assert(ErrorCode::FileNotFound == ErrorCode::kFileNotFound, "");
static_assert(ErrorCode::KvsFileReadError == ErrorCode::kKvsFileReadError, "");
static_assert(ErrorCode::KvsHashFileReadError == ErrorCode::kKvsHashFileReadError, "");
static_assert(ErrorCode::JsonParserError == ErrorCode::kJsonParserError, "");
static_assert(ErrorCode::JsonGeneratorError == ErrorCode::kJsonGeneratorError, "");
static_assert(ErrorCode::PhysicalStorageFailure == ErrorCode::kPhysicalStorageFailure, "");
static_assert(ErrorCode::IntegrityCorrupted == ErrorCode::kIntegrityCorrupted, "");
static_assert(ErrorCode::ValidationFailed == ErrorCode::kValidationFailed, "");
static_assert(ErrorCode::EncryptionFailed == ErrorCode::kEncryptionFailed, "");
static_assert(ErrorCode::ResourceBusy == ErrorCode::kResourceBusy, "");
static_assert(ErrorCode::OutOfStorageSpace == ErrorCode::kOutOfStorageSpace, "");
static_assert(ErrorCode::QuotaExceeded == ErrorCode::kQuotaExceeded, "");
static_assert(ErrorCode::AuthenticationFailed == ErrorCode::kAuthenticationFailed, "");
static_assert(ErrorCode::KeyNotFound == ErrorCode::kKeyNotFound, "");
static_assert(ErrorCode::KeyDefaultNotFound == ErrorCode::kKeyDefaultNotFound, "");
static_assert(ErrorCode::SerializationFailed == ErrorCode::kSerializationFailed, "");
static_assert(ErrorCode::InvalidSnapshotId == ErrorCode::kInvalidSnapshotId, "");
static_assert(ErrorCode::ConversionFailed == ErrorCode::kConversionFailed, "");
static_assert(ErrorCode::MutexLockFailed == ErrorCode::kMutexLockFailed, "");
static_assert(ErrorCode::InvalidValueType == ErrorCode::kInvalidValueType, "");
static_assert(ErrorCode::InvalidKey == ErrorCode::kInvalidKey, "");
DISABLE_WARNING_POP

score::Result<Kvs> OpenKvs()
{
    return Kvs::Open(kInstanceId,
                     kSnapshotId,
                     OpenNeedDefaults::kOptional,
                     OpenNeedKvs::kOptional,
                     std::string(kDataDir),
                     std::nullopt,
                     kDefaultMaxSnapshots);
}

void ExpectSame(const score::Result<void>& old_result, const score::Result<void>& new_result)
{
    ASSERT_EQ(old_result.has_value(), new_result.has_value());
    if (!old_result.has_value())
    {
        EXPECT_EQ(*old_result.error(), *new_result.error());
    }
}

}  // namespace

TEST(kvs_deprecated_api, kvs_open)
{
    PrepareEnvironment();
    // clang-format off
    DISABLE_WARNING_PUSH
    DISABLE_WARNING(-Wdeprecated-declarations)
    // clang-format on
    auto result = Kvs::open(kInstanceId,
                            kSnapshotId,
                            OpenNeedDefaults::kOptional,
                            OpenNeedKvs::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    DISABLE_WARNING_POP
    auto new_result = Kvs::Open(kInstanceId,
                                kSnapshotId,
                                OpenNeedDefaults::kOptional,
                                OpenNeedKvs::kOptional,
                                std::string(kDataDir),
                                std::nullopt,
                                kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    ASSERT_TRUE(new_result);
    EXPECT_EQ(result.value().GetAllKeys().value(), new_result.value().GetAllKeys().value());
    EXPECT_EQ(result.value().SnapshotMaxCount(), new_result.value().SnapshotMaxCount());
    EXPECT_STREQ(result.value().GetKvsFilename(kSnapshotId).value().CStr(),
                 new_result.value().GetKvsFilename(kSnapshotId).value().CStr());
    CleanupEnvironment();
}

TEST(kvs_deprecated_api, kvs_read_wrappers)
{
    PrepareEnvironment();
    auto result = OpenKvs();
    ASSERT_TRUE(result);
    Kvs& kvs = result.value();

    // clang-format off
    DISABLE_WARNING_PUSH
    DISABLE_WARNING(-Wdeprecated-declarations)
    // clang-format on
    auto old_keys = kvs.get_all_keys();
    auto old_exists = kvs.key_exists("kvs");
    auto old_missing = kvs.key_exists("missing");
    auto old_value = kvs.get_value("kvs");
    auto old_default = kvs.get_default_value("default");
    auto old_is_default = kvs.is_value_default("default");
    auto old_count = kvs.snapshot_count();
    const size_t old_max_count = kvs.snapshot_max_count();
    auto old_kvs_filename = kvs.get_kvs_filename(kSnapshotId);
    auto old_hash_filename = kvs.get_hash_filename(kSnapshotId);
    auto old_file_size = kvs.get_storage_file_size();
    auto old_potential_size = kvs.calculate_potential_size();
    DISABLE_WARNING_POP

    auto new_keys = kvs.GetAllKeys();
    ASSERT_TRUE(old_keys);
    ASSERT_TRUE(new_keys);
    EXPECT_EQ(old_keys.value(), new_keys.value());

    EXPECT_EQ(old_exists.value(), kvs.KeyExists("kvs").value());
    EXPECT_EQ(old_missing.value(), kvs.KeyExists("missing").value());

    ASSERT_TRUE(old_value);
    EXPECT_EQ(old_value.value().GetType(), kvs.GetValue("kvs").value().GetType());
    EXPECT_EQ(std::get<int32_t>(old_value.value().GetValue()),
              std::get<int32_t>(kvs.GetValue("kvs").value().GetValue()));

    ASSERT_TRUE(old_default);
    EXPECT_EQ(std::get<int32_t>(old_default.value().GetValue()),
              std::get<int32_t>(kvs.GetDefaultValue("default").value().GetValue()));

    EXPECT_EQ(old_is_default.value(), kvs.IsValueDefault("default").value());
    EXPECT_EQ(old_count.value(), kvs.SnapshotCount().value());
    EXPECT_EQ(old_max_count, kvs.SnapshotMaxCount());
    EXPECT_STREQ(old_kvs_filename.value().CStr(), kvs.GetKvsFilename(kSnapshotId).value().CStr());
    EXPECT_STREQ(old_hash_filename.value().CStr(), kvs.GetHashFilename(kSnapshotId).value().CStr());
    EXPECT_EQ(old_file_size.value(), kvs.GetStorageFileSize().value());
    EXPECT_EQ(old_potential_size.value(), kvs.CalculatePotentialSize().value());
    CleanupEnvironment();
}

TEST(kvs_deprecated_api, kvs_write_wrappers)
{
    PrepareEnvironment();
    auto result = OpenKvs();
    ASSERT_TRUE(result);
    Kvs& kvs = result.value();

    // clang-format off
    DISABLE_WARNING_PUSH
    DISABLE_WARNING(-Wdeprecated-declarations)
    // clang-format on
    // Each old wrapper is called once and its effect is checked, so that a wrapper forwarding elsewhere is caught.
    ExpectSame(kvs.set_value("old_key", KvsValue(1)), kvs.SetValue("new_key", KvsValue(1)));
    EXPECT_EQ(std::get<int32_t>(kvs.GetValue("old_key").value().GetValue()), 1);
    EXPECT_EQ(std::get<int32_t>(kvs.GetValue("new_key").value().GetValue()), 1);

    ASSERT_TRUE(kvs.SetValue("default", KvsValue(9)));
    ASSERT_FALSE(kvs.IsValueDefault("default").value());
    ASSERT_TRUE(kvs.reset_key("default"));
    EXPECT_TRUE(kvs.IsValueDefault("default").value());

    ASSERT_TRUE(kvs.remove_key("old_key"));
    EXPECT_FALSE(kvs.KeyExists("old_key").value());
    EXPECT_TRUE(kvs.KeyExists("new_key").value());

    // A flushed key is visible to a KVS opened afterwards.
    ASSERT_TRUE(kvs.flush());
    auto reopened = OpenKvs();
    ASSERT_TRUE(reopened);
    EXPECT_TRUE(reopened.value().KeyExists("new_key").value());
    EXPECT_TRUE(reopened.value().KeyExists("kvs").value());

    // Pending changes are dropped, flushed ones stay.
    ASSERT_TRUE(kvs.SetValue("pending", KvsValue(3)));
    ASSERT_TRUE(kvs.discard_pending_changes());
    EXPECT_FALSE(kvs.KeyExists("pending").value());
    EXPECT_TRUE(kvs.KeyExists("new_key").value());
    EXPECT_TRUE(kvs.KeyExists("kvs").value());

    ASSERT_TRUE(kvs.remove_all_keys());
    EXPECT_TRUE(kvs.GetAllKeys().value().empty());

    ASSERT_TRUE(kvs.SetValue("again", KvsValue(4)));
    ASSERT_TRUE(kvs.reset());
    EXPECT_TRUE(kvs.GetAllKeys().value().empty());

    auto old_restore_ok = kvs.snapshot_restore(SnapshotId{0ul});
    auto new_restore_ok = kvs.SnapshotRestore(SnapshotId{0ul});
    ExpectSame(old_restore_ok, new_restore_ok);
    auto old_restore_bad = kvs.snapshot_restore(SnapshotId{99ul});
    auto new_restore_bad = kvs.SnapshotRestore(SnapshotId{99ul});
    ExpectSame(old_restore_bad, new_restore_bad);
    DISABLE_WARNING_POP
    CleanupEnvironment();
}

TEST(kvs_deprecated_api, kvsbuilder_wrappers)
{
    PrepareEnvironment();

    // clang-format off
    DISABLE_WARNING_PUSH
    DISABLE_WARNING(-Wdeprecated-declarations)
    // clang-format on
    KvsBuilder old_builder(kInstanceId);
    old_builder.snapshot(SnapshotId{23ul})
        .need_defaults_flag(OpenNeedDefaults::kRequired)
        .need_kvs_flag(OpenNeedKvs::kRequired)
        .dir("./kvsbuilder/")
        .max_storage_bytes(std::optional<size_t>{1024ul})
        .snapshot_max_count(2ul);

    KvsBuilder new_builder(kInstanceId);
    new_builder.Snapshot(SnapshotId{23ul})
        .NeedDefaultsFlag(OpenNeedDefaults::kRequired)
        .NeedKvsFlag(OpenNeedKvs::kRequired)
        .Dir("./kvsbuilder/")
        .MaxStorageBytes(std::optional<size_t>{1024ul})
        .SnapshotMaxCount(2ul);
    DISABLE_WARNING_POP

    EXPECT_EQ(old_builder.snapshot_id_.id, new_builder.snapshot_id_.id);
    EXPECT_EQ(old_builder.need_defaults_, new_builder.need_defaults_);
    EXPECT_EQ(old_builder.need_kvs_, new_builder.need_kvs_);
    EXPECT_EQ(old_builder.directory_, new_builder.directory_);
    EXPECT_EQ(old_builder.maximum_storage_bytes_, new_builder.maximum_storage_bytes_);
    EXPECT_EQ(old_builder.max_snapshots_, new_builder.max_snapshots_);

    // Both builders fail the same way, since the required files are not available.
    // clang-format off
    DISABLE_WARNING_PUSH
    DISABLE_WARNING(-Wdeprecated-declarations)
    // clang-format on
    auto old_result = old_builder.build();
    DISABLE_WARNING_POP
    auto new_result = new_builder.Build();
    ASSERT_FALSE(old_result);
    ASSERT_FALSE(new_result);
    EXPECT_EQ(*old_result.error(), *new_result.error());

    // clang-format off
    DISABLE_WARNING_PUSH
    DISABLE_WARNING(-Wdeprecated-declarations)
    // clang-format on
    auto success = KvsBuilder(kInstanceId).dir(std::string(kDataDir)).build();
    DISABLE_WARNING_POP
    auto new_success = KvsBuilder(kInstanceId).Dir(std::string(kDataDir)).Build();
    ASSERT_TRUE(success);
    ASSERT_TRUE(new_success);
    EXPECT_EQ(success.value().GetAllKeys().value(), new_success.value().GetAllKeys().value());
    EXPECT_EQ(success.value().SnapshotMaxCount(), new_success.value().SnapshotMaxCount());
    CleanupEnvironment();
}

TEST(kvs_deprecated_api, kvsvalue_wrappers)
{
    const KvsValue value(42);

    // clang-format off
    DISABLE_WARNING_PUSH
    DISABLE_WARNING(-Wdeprecated-declarations)
    // clang-format on
    EXPECT_EQ(value.getType(), value.GetType());
    EXPECT_EQ(std::get<int32_t>(value.getValue()), std::get<int32_t>(value.GetValue()));
    DISABLE_WARNING_POP
}

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
#include "score/os/ObjectSeam.h"
#include "score/os/mocklib/fcntl_mock.h"
#include "score/os/mocklib/stat_mock.h"
#include "score/os/stat.h"
#include "score/utility.hpp"

#include <unistd.h>

TEST(kvs_constructor, move_constructor)
{
    const std::uint32_t instance_b = 5;

    // Create object A.
    auto result_a = Kvs::Open(InstanceId(instance_b),
                              kSnapshotId,
                              KvsDefaults::kOptional,
                              KvsLoad::kOptional,
                              std::string(kDataDir),
                              std::nullopt,
                              kDefaultMaxSnapshots);
    ASSERT_TRUE(result_a);
    Kvs kvs_a = std::move(result_a.value());

    // Create object B.
    auto result_b = Kvs::Open(kInstanceId,
                              kSnapshotId,
                              KvsDefaults::kOptional,
                              KvsLoad::kOptional,
                              std::string(kDataDir),
                              std::nullopt,
                              kDefaultMaxSnapshots);
    ASSERT_TRUE(result_b);
    Kvs kvs_b = std::move(result_b.value());

    // Create test data.
    kvs_b.kvs_.insert({"test_kvs", KvsValue(42.0)});
    kvs_b.default_values_.insert({"test_default", KvsValue(true)});

    // Move assignment operator.
    kvs_a = std::move(kvs_b);

// Ignore compiler warning "-Wself-move" for GCC and Clang.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#endif
    // Intentional self-move.
    kvs_a = std::move(kvs_a);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    // Expectations:
    // - kvs_a now contains data from the former kvs_b (default JSON data).

    EXPECT_EQ(kvs_a.filename_prefix_.CStr(), kDataDir + "kvs_" + std::to_string(kInstance));

    EXPECT_TRUE(kvs_a.kvs_.count("test_kvs"));
    EXPECT_TRUE(kvs_a.default_values_.count("test_default"));

    const auto& val = kvs_a.kvs_.at("test_kvs");
    EXPECT_EQ(val.GetType(), KvsValue::Type::kF64);
    EXPECT_EQ(std::get<double>(val.GetValue()), 42.0);

    const auto& def = kvs_a.default_values_.at("test_default");
    EXPECT_EQ(def.GetType(), KvsValue::Type::kBoolean);
    EXPECT_EQ(std::get<bool>(def.GetValue()), true);

    EXPECT_TRUE(kvs_b.kvs_.empty());
    EXPECT_TRUE(kvs_b.default_values_.empty());

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_sucess)
{
    // Success.
    PrepareEnvironment();

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    auto mock_parser = std::make_unique<score::json::IJsonParserMock>();
    score::json::Object obj;
    score::json::Object inner_obj;
    inner_obj.emplace("t", score::json::Any(std::string("i32")));
    inner_obj.emplace("v", score::json::Any(42));
    obj.emplace("kvs", score::json::Any(std::move(inner_obj)));
    score::json::Any any_obj(std::move(obj));

    EXPECT_CALL(*mock_parser, FromBuffer(::testing::_))
        .WillOnce(::testing::Return(score::Result<score::json::Any>(std::move(any_obj))));

    kvs->parser_ = std::move(mock_parser);

    auto result = kvs->ParseJsonData("data_not_used_in_mocking");
    EXPECT_TRUE(result);

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_failure)
{
    PrepareEnvironment();

    // JSON parser failure.

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    auto mock_parser = std::make_unique<score::json::IJsonParserMock>();
    EXPECT_CALL(*mock_parser, FromBuffer(::testing::_))
        .WillOnce(::testing::Return(
            score::Result<score::json::Any>(score::MakeUnexpected(score::json::Error::kInvalidFilePath))));

    kvs->parser_ = std::move(mock_parser);

    auto result = kvs->ParseJsonData("data_not_used_in_mocking");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kJsonParserError);

    // No object returned failure.

    mock_parser = std::make_unique<score::json::IJsonParserMock>();
    score::json::Any json_parser_return_value(42.0);

    EXPECT_CALL(*mock_parser, FromBuffer(::testing::_))
        .WillOnce(::testing::Return(score::Result<score::json::Any>(std::move(json_parser_return_value))));

    kvs->parser_ = std::move(mock_parser);

    result = kvs->ParseJsonData("data_not_used_in_mocking");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kJsonParserError);

    // AnyToKvsValue error.

    mock_parser = std::make_unique<score::json::IJsonParserMock>();
    score::json::Object obj;
    score::json::Object inner_obj;
    inner_obj.emplace("t", score::json::Any(std::string("invalid")));
    inner_obj.emplace("v", score::json::Any(42));
    obj.emplace("kvs", score::json::Any(std::move(inner_obj)));
    score::json::Any any_obj(std::move(obj));

    EXPECT_CALL(*mock_parser, FromBuffer(::testing::_))
        .WillOnce(::testing::Return(score::Result<score::json::Any>(std::move(any_obj))));

    kvs->parser_ = std::move(mock_parser);

    result = kvs->ParseJsonData("data_not_used_in_mocking");
    EXPECT_FALSE(result.has_value());
    // Error is passed from AnyToKvsValue.
    EXPECT_EQ(result.error(), ErrorCode::kInvalidValueType);

    CleanupEnvironment();
}

TEST(kvs_open, storage_corrupted_all_kvs_open_modes)
{
    const KvsDefaults default_mode{KvsDefaults::kRequired};
    const InstanceId instance_id{99ul};
    const SnapshotId snapshot_id{0ul};
    const std::string default_data = R"({"default": {"t": "i32","v": 10}})";
    const std::string snapshot_data_0 = R"({"kvs": {"t": "i32","v": 20}})";
    const std::string dir{"./kvs/"};
    const std::string prefix{"kvs_"};

    // Prepare KVS on disk.
    // Make sure files have been created.
    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0);
    ASSERT_TRUE(std::filesystem::exists(dir + prefix + std::to_string(instance_id.id) + "_0.json"));
    ASSERT_TRUE(std::filesystem::exists(dir + prefix + std::to_string(instance_id.id) + "_0.hash"));

    // Corrupt JSON file.
    {
        const std::string json_path{dir + prefix + std::to_string(instance_id.id) + "_0.json"};
        const std::string corrupted_data{"random_data"};
        std::ofstream file(json_path);
        if (file.is_open())
        {
            file << corrupted_data;
            file.close();
        }
    }

    // Required.
    {
        auto kvs = Kvs::Open(instance_id,
                             snapshot_id,
                             default_mode,
                             KvsLoad::kRequired,
                             std::string(dir),
                             std::nullopt,
                             kDefaultMaxSnapshots);
        ASSERT_FALSE(kvs);
        ASSERT_EQ(kvs.error(), ErrorCode::kValidationFailed);
    }

    // Optional.
    {
        auto kvs = Kvs::Open(instance_id,
                             snapshot_id,
                             default_mode,
                             KvsLoad::kOptional,
                             std::string(dir),
                             std::nullopt,
                             kDefaultMaxSnapshots);
        ASSERT_FALSE(kvs);
        ASSERT_EQ(kvs.error(), ErrorCode::kValidationFailed);
    }

    // Ignored.
    {
        auto kvs = Kvs::Open(instance_id,
                             snapshot_id,
                             default_mode,
                             KvsLoad::kIgnored,
                             std::string(dir),
                             std::nullopt,
                             kDefaultMaxSnapshots);

        // Successfully opened without loading.
        ASSERT_TRUE(kvs);

        // `kvs` should be not present.
        ASSERT_FALSE(kvs->GetValue("kvs"));

        // No keys available.
        auto all_keys = kvs->GetAllKeys();
        ASSERT_TRUE(all_keys);
        ASSERT_EQ(all_keys->size(), 0ul);
    }

    CleanupEnvironment(dir);
}

TEST(kvs_open, hash_missing_all_kvs_open_modes)
{
    const KvsDefaults default_mode{KvsDefaults::kRequired};
    const InstanceId instance_id{99ul};
    const SnapshotId snapshot_id{0ul};
    const std::string default_data = R"({"default": {"t": "i32","v": 10}})";
    const std::string snapshot_data_0 = R"({"kvs": {"t": "i32","v": 20}})";
    const std::string dir{"./kvs/"};
    const std::string prefix{"kvs_"};

    // Prepare KVS on disk.
    // Make sure files have been created.
    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0);
    ASSERT_TRUE(std::filesystem::exists(dir + prefix + std::to_string(instance_id.id) + "_0.json"));
    ASSERT_TRUE(std::filesystem::exists(dir + prefix + std::to_string(instance_id.id) + "_0.hash"));
    // Remove hash file.
    ASSERT_TRUE(std::filesystem::remove(dir + prefix + std::to_string(instance_id.id) + "_0.hash"));

    // Required.
    {
        auto kvs = Kvs::Open(instance_id,
                             snapshot_id,
                             default_mode,
                             KvsLoad::kRequired,
                             std::string(dir),
                             std::nullopt,
                             kDefaultMaxSnapshots);
        ASSERT_FALSE(kvs);
        ASSERT_EQ(kvs.error(), ErrorCode::kKvsHashFileReadError);
    }

    // Optional.
    {
        auto kvs = Kvs::Open(instance_id,
                             snapshot_id,
                             default_mode,
                             KvsLoad::kOptional,
                             std::string(dir),
                             std::nullopt,
                             kDefaultMaxSnapshots);
        ASSERT_FALSE(kvs);
        ASSERT_EQ(kvs.error(), ErrorCode::kKvsHashFileReadError);
    }

    // Ignored.
    {
        auto kvs = Kvs::Open(instance_id,
                             snapshot_id,
                             default_mode,
                             KvsLoad::kIgnored,
                             std::string(dir),
                             std::nullopt,
                             kDefaultMaxSnapshots);
        // Successfully opened without loading.
        ASSERT_TRUE(kvs);
        // `kvs` should be not present.
        ASSERT_FALSE(kvs->GetValue("kvs"));
        // No keys available.
        auto all_keys = kvs->GetAllKeys();
        ASSERT_TRUE(all_keys);
        ASSERT_EQ(all_keys->size(), 0ul);
    }

    CleanupEnvironment(dir);
}

TEST(kvs_open, selected_snapshot)
{
    const InstanceId instance_id{99ul};
    const std::string default_data = R"({"default": {"t": "i32","v": 10}})";
    const std::string snapshot_data_0 = R"({"kvs": {"t": "i32","v": 20}})";
    const std::string snapshot_data_1 = R"({"kvs": {"t": "i32","v": 30}})";
    const std::string dir{"./kvs/"};
    const std::string prefix{"kvs_"};

    // Prepare KVS on disk with two snapshots.
    // Make sure files have been created.
    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0, snapshot_data_1);
    EXPECT_TRUE(std::filesystem::exists(dir + prefix + std::to_string(instance_id.id) + "_0.json"));
    EXPECT_TRUE(std::filesystem::exists(dir + prefix + std::to_string(instance_id.id) + "_1.json"));

    // First instance, load snapshot 0.
    {
        auto kvs = Kvs::Open(instance_id,
                             SnapshotId{0ul},
                             KvsDefaults::kRequired,
                             KvsLoad::kRequired,
                             std::string(dir),
                             std::nullopt,
                             kDefaultMaxSnapshots);
        ASSERT_TRUE(kvs);
        auto value = kvs->GetValue("kvs");
        ASSERT_TRUE(value);
        EXPECT_EQ(value.value().GetType(), KvsValue::Type::kI32);
        EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 20u);
    }

    // Second instance, load snapshot 1.
    {
        auto kvs = Kvs::Open(instance_id,
                             SnapshotId{1ul},
                             KvsDefaults::kRequired,
                             KvsLoad::kRequired,
                             std::string(dir),
                             std::nullopt,
                             kDefaultMaxSnapshots);
        ASSERT_TRUE(kvs);
        auto value = kvs->GetValue("kvs");
        ASSERT_TRUE(value);
        EXPECT_EQ(value.value().GetType(), KvsValue::Type::kI32);
        EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 30u);
    }

    CleanupEnvironment(dir);
}

TEST(kvs_TEST, parse_json_data_rejects_over_length_key)
{
    // comp_req__kvs__key_length: an over-length key found while loading means the file is invalid or corrupted.
    // Loading must halt with an error and not silently drop data.
    PrepareEnvironment();

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    const std::string valid_key(kMaxKeyLengthBytes, 'a');
    const std::string too_long_key(kMaxKeyLengthBytes + 1U, 'b');

    auto mock_parser = std::make_unique<score::json::IJsonParserMock>();
    score::json::Object obj;

    score::json::Object valid_inner;
    valid_inner.emplace("t", score::json::Any(std::string("i32")));
    valid_inner.emplace("v", score::json::Any(42));
    obj.emplace(valid_key, score::json::Any(std::move(valid_inner)));

    score::json::Object long_inner;
    long_inner.emplace("t", score::json::Any(std::string("i32")));
    long_inner.emplace("v", score::json::Any(7));
    obj.emplace(too_long_key, score::json::Any(std::move(long_inner)));

    score::json::Any any_obj(std::move(obj));
    EXPECT_CALL(*mock_parser, FromBuffer(::testing::_))
        .WillOnce(::testing::Return(score::Result<score::json::Any>(std::move(any_obj))));

    kvs->parser_ = std::move(mock_parser);

    auto result = kvs->ParseJsonData("data_not_used_in_mocking");
    ASSERT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kInvalidKey);

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_rejects_empty_key)
{
    // comp_req__kvs__key_length: an empty key found while loading means the file is invalid or corrupted.
    // Loading must halt with an error.
    PrepareEnvironment();

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    auto mock_parser = std::make_unique<score::json::IJsonParserMock>();
    score::json::Object obj;

    score::json::Object valid_inner;
    valid_inner.emplace("t", score::json::Any(std::string("i32")));
    valid_inner.emplace("v", score::json::Any(42));
    obj.emplace("valid_key", score::json::Any(std::move(valid_inner)));

    score::json::Object empty_inner;
    empty_inner.emplace("t", score::json::Any(std::string("i32")));
    empty_inner.emplace("v", score::json::Any(7));
    obj.emplace("", score::json::Any(std::move(empty_inner)));

    score::json::Any any_obj(std::move(obj));
    EXPECT_CALL(*mock_parser, FromBuffer(::testing::_))
        .WillOnce(::testing::Return(score::Result<score::json::Any>(std::move(any_obj))));

    kvs->parser_ = std::move(mock_parser);

    auto result = kvs->ParseJsonData("data_not_used_in_mocking");
    ASSERT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kInvalidKey);

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_rejects_invalid_characters_key)
{
    // comp_req__kvs__key_naming: a key with characters outside printable ASCII means the file is invalid or corrupted.
    PrepareEnvironment();

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    const std::vector<std::string> invalid_keys{"\x1f", "\x7f", "tab\tkey", u8"ą", "\x80"};
    for (const auto& invalid_key : invalid_keys)
    {
        auto mock_parser = std::make_unique<score::json::IJsonParserMock>();
        score::json::Object obj;

        score::json::Object valid_inner;
        valid_inner.emplace("t", score::json::Any(std::string("i32")));
        valid_inner.emplace("v", score::json::Any(42));
        obj.emplace("valid_key", score::json::Any(std::move(valid_inner)));

        score::json::Object invalid_inner;
        invalid_inner.emplace("t", score::json::Any(std::string("i32")));
        invalid_inner.emplace("v", score::json::Any(7));
        obj.emplace(invalid_key, score::json::Any(std::move(invalid_inner)));

        score::json::Any any_obj(std::move(obj));
        EXPECT_CALL(*mock_parser, FromBuffer(::testing::_))
            .WillOnce(::testing::Return(score::Result<score::json::Any>(std::move(any_obj))));

        kvs->parser_ = std::move(mock_parser);

        auto result = kvs->ParseJsonData("data_not_used_in_mocking");
        ASSERT_FALSE(result) << "key: " << invalid_key;
        EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kInvalidKey) << "key: " << invalid_key;
    }

    CleanupEnvironment();
}

TEST(kvs_open_json, open_json_success)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    auto result = kvs->OpenJson(score::filesystem::Path(kKvsPrefix), OpenJsonNeedFile::kRequired);
    EXPECT_TRUE(result);
    result = kvs->OpenJson(score::filesystem::Path(kKvsPrefix), OpenJsonNeedFile::kOptional);
    EXPECT_TRUE(result);

    CleanupEnvironment();
}

TEST(kvs_open_json, open_json_json_invalid)
{
    PrepareEnvironment();
    // JSON data invalid (ParseJsonData() failure).
    std::string invalid_json = "{ invalid json }";
    std::ofstream invalid_json_file(kKvsPrefix + ".json");
    invalid_json_file << invalid_json;
    invalid_json_file.close();

    // Still valid hash.
    uint32_t kvs_hash = Adler32(invalid_json);
    std::ofstream kvs_hash_file(kKvsPrefix + ".hash", std::ios::binary);
    kvs_hash_file.put((kvs_hash >> 24) & 0xFF);
    kvs_hash_file.put((kvs_hash >> 16) & 0xFF);
    kvs_hash_file.put((kvs_hash >> 8) & 0xFF);
    kvs_hash_file.put(kvs_hash & 0xFF);
    kvs_hash_file.close();

    // Create Kvs instance without any data (this constructor is normally private).
    Kvs kvs = Kvs();

    // Make JSON parser fail.
    auto mock_parser = std::make_unique<score::json::IJsonParserMock>();
    EXPECT_CALL(*mock_parser, FromBuffer(::testing::_)).WillRepeatedly([](auto) {
        return score::Result<score::json::Any>(score::MakeUnexpected(score::json::Error::kInvalidFilePath));
    });
    kvs.parser_ = std::move(mock_parser);

    auto result = kvs.OpenJson(score::filesystem::Path(kKvsPrefix), OpenJsonNeedFile::kRequired);
    ASSERT_FALSE(result);
    // Error code passed by ParseJsonData().
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kJsonParserError);

    // JSON not existing.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    result = kvs.OpenJson(score::filesystem::Path(kKvsPrefix), OpenJsonNeedFile::kRequired);
    ASSERT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kKvsFileReadError);

    CleanupEnvironment();
}

TEST(kvs_open_json, open_json_hash_invalid)
{
    PrepareEnvironment();
    // Hash corrupted.
    std::fstream corrupt_default_hash_file(kKvsPrefix + ".hash", std::ios::in | std::ios::out | std::ios::binary);
    corrupt_default_hash_file.seekp(0);
    corrupt_default_hash_file.put(0xFF);
    corrupt_default_hash_file.close();

    // Create Kvs instance without any data (this constructor is normally private).
    Kvs kvs = Kvs();

    auto result = kvs.OpenJson(score::filesystem::Path(kKvsPrefix), OpenJsonNeedFile::kOptional);
    ASSERT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kValidationFailed);

    // Hash not existing.
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());
    result = kvs.OpenJson(score::filesystem::Path(kKvsPrefix), OpenJsonNeedFile::kOptional);
    ASSERT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kKvsHashFileReadError);

    // JSON not existing.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    result = kvs.OpenJson(score::filesystem::Path(kKvsPrefix), OpenJsonNeedFile::kRequired);
    ASSERT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kKvsFileReadError);

    CleanupEnvironment();
}

TEST(kvs_reset, reset_success)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Check that data exists.
    EXPECT_FALSE(result.value().kvs_.empty());

    // Reset KVS.
    auto reset_result = result.value().Reset();
    EXPECT_TRUE(reset_result);
    EXPECT_TRUE(result.value().kvs_.empty());

    CleanupEnvironment();
}

TEST(kvs_reset, reset_failure)
{
    PrepareEnvironment();

    // Mutex locked.
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    auto reset_result = result.value().Reset();
    EXPECT_FALSE(reset_result);
    EXPECT_EQ(static_cast<ErrorCode>(*reset_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_get_all_keys, get_all_keys_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Check that data exists.
    EXPECT_FALSE(result.value().kvs_.empty());

    // Check if all keys are returned.
    auto get_all_keys_result = result.value().GetAllKeys();
    ASSERT_TRUE(get_all_keys_result);
    EXPECT_FALSE(get_all_keys_result.value().empty());

    auto search = std::find(get_all_keys_result.value().begin(), get_all_keys_result.value().end(), "kvs");
    EXPECT_TRUE(search != get_all_keys_result.value().end());

    // Check if empty keys are returned.
    result.value().kvs_.clear();
    get_all_keys_result = result.value().GetAllKeys();
    EXPECT_TRUE(get_all_keys_result.value().empty());

    CleanupEnvironment();
}

TEST(kvs_get_all_keys, get_all_keys_failure)
{
    PrepareEnvironment();

    // Mutex locked.
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);

    auto get_all_keys_result = result.value().GetAllKeys();
    EXPECT_FALSE(get_all_keys_result);
    EXPECT_EQ(static_cast<ErrorCode>(*get_all_keys_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_key_exists, key_exists_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Check that data exists.
    EXPECT_FALSE(result.value().kvs_.empty());

    // Check if key exists.
    auto exists_result = result.value().KeyExists("kvs");
    EXPECT_TRUE(exists_result);
    EXPECT_TRUE(exists_result.value());
    // Check if non-existing key returns false.
    exists_result = result.value().KeyExists("non_existing_key");
    EXPECT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());

    CleanupEnvironment();
}

TEST(kvs_key_exists, key_exists_failure)
{
    PrepareEnvironment();

    // Mutex locked.
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    auto exists_result = result.value().KeyExists("kvs");
    EXPECT_FALSE(exists_result);
    EXPECT_EQ(static_cast<ErrorCode>(*exists_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Check that data exists.
    EXPECT_FALSE(result.value().kvs_.empty());

    // Check if value is returned.
    auto get_value_result = result.value().GetValue("kvs");
    ASSERT_TRUE(get_value_result);
    EXPECT_EQ(get_value_result.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(get_value_result.value().GetValue()), 2);

    // Check if default value is returned when no written key exists.
    result.value().kvs_.clear();
    // Make sure kvs is empty and it uses the default value.
    ASSERT_TRUE(result.value().kvs_.empty());
    int32_t default_value(42);
    result.value().default_values_.insert_or_assign("kvs", KvsValue(default_value));
    get_value_result = result.value().GetValue("kvs");
    ASSERT_FALSE(get_value_result);
    EXPECT_EQ(get_value_result.error(), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_failure)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Check if non-existing key returns error.
    auto get_value_result = result.value().GetValue("non_existing_key");
    EXPECT_FALSE(get_value_result);
    EXPECT_EQ(get_value_result.error(), ErrorCode::kKeyNotFound);

    // Mutex locked.
    result = Kvs::Open(kInstanceId,
                       kSnapshotId,
                       KvsDefaults::kRequired,
                       KvsLoad::kRequired,
                       std::string(kDataDir),
                       std::nullopt,
                       kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    get_value_result = result.value().GetValue("kvs");
    EXPECT_FALSE(get_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*get_value_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_get_default_value, get_default_value_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Check that data exists.
    int32_t default_value(42);
    result.value().default_values_.insert_or_assign("kvs", KvsValue(default_value));

    // Check if default value is returned.
    auto get_def_value_result = result.value().GetDefaultValue("kvs");
    ASSERT_TRUE(get_def_value_result);
    EXPECT_EQ(get_def_value_result.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(get_def_value_result.value().GetValue()), 42);

    CleanupEnvironment();
}

TEST(kvs_get_default_value, get_default_value_failure)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Check if non-existing key returns error.
    auto get_def_value_result = result.value().GetDefaultValue("non_existing_key");
    EXPECT_FALSE(get_def_value_result);
    EXPECT_EQ(get_def_value_result.error(), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_reset_key, reset_key_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    // Check that data exists.
    ASSERT_TRUE(result.value().kvs_.count("kvs"));

    // Create default value for "kvs" key.
    result.value().default_values_.insert_or_assign("kvs", KvsValue(42.0));
    // Reset a key.
    auto reset_key_result = result.value().ResetKey("kvs");
    EXPECT_TRUE(reset_key_result);
    EXPECT_FALSE(result.value().kvs_.count("kvs"));
    EXPECT_TRUE(result.value().default_values_.count("kvs"));

    // Reset a non written key which has default value.
    result.value().default_values_.insert_or_assign("default", KvsValue(42.0));
    reset_key_result = result.value().ResetKey("default");
    EXPECT_TRUE(reset_key_result);

    CleanupEnvironment();
}

TEST(kvs_reset_key, reset_key_failure)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Reset a non-existing key.
    auto reset_key_result = result.value().ResetKey("non_existing_key");
    EXPECT_FALSE(reset_key_result);
    EXPECT_EQ(reset_key_result.error(), ErrorCode::kKeyDefaultNotFound);

    // Reset a key without default value.
    result = Kvs::Open(kInstanceId,
                       kSnapshotId,
                       KvsDefaults::kRequired,
                       KvsLoad::kRequired,
                       std::string(kDataDir),
                       std::nullopt,
                       kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    // Clear default values to ensure no default value exists for "kvs".
    result.value().default_values_.clear();
    reset_key_result = result.value().ResetKey("kvs");
    EXPECT_FALSE(reset_key_result);
    EXPECT_EQ(reset_key_result.error(), ErrorCode::kKeyDefaultNotFound);

    // Mutex locked.
    result = Kvs::Open(kInstanceId,
                       kSnapshotId,
                       KvsDefaults::kRequired,
                       KvsLoad::kRequired,
                       std::string(kDataDir),
                       std::nullopt,
                       kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    reset_key_result = result.value().ResetKey("kvs");
    EXPECT_FALSE(reset_key_result);
    EXPECT_EQ(static_cast<ErrorCode>(*reset_key_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_is_value_default, is_value_default)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    auto kvs{std::move(result.value())};

    // Create test default data.
    kvs.kvs_.insert_or_assign("not-default", KvsValue(123.4));
    kvs.default_values_.insert_or_assign("default", KvsValue(42.0));

    // Check non-default value.
    {
        auto result{kvs.IsValueDefault("not-default")};
        ASSERT_TRUE(result);
        ASSERT_FALSE(result.value());
    }

    // Check default value.
    {
        auto result{kvs.IsValueDefault("default")};
        ASSERT_TRUE(result);
        ASSERT_TRUE(result.value());
    }

    // Check key not found.
    {
        auto result{kvs.IsValueDefault("non_existing_key")};
        ASSERT_FALSE(result);
        ASSERT_EQ(result.error(), ErrorCode::kKeyNotFound);
    }

    CleanupEnvironment();
}

TEST(kvs_is_value_default, is_value_default_failure)
{
    PrepareEnvironment();

    // Mutex locked.
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    auto default_result = result.value().IsValueDefault("default");
    EXPECT_FALSE(default_result);
    EXPECT_EQ(static_cast<ErrorCode>(*default_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_success)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Set a new value.
    auto set_value_result = result.value().SetValue("new_key", KvsValue(3.14));
    EXPECT_TRUE(set_value_result);
    EXPECT_TRUE(result.value().kvs_.count("new_key"));
    EXPECT_EQ(result.value().kvs_.at("new_key").GetType(), KvsValue::Type::kF64);
    EXPECT_DOUBLE_EQ(std::get<double>(result.value().kvs_.at("new_key").GetValue()), 3.14);

    // Set a value with an existing key.
    set_value_result = result.value().SetValue("kvs", KvsValue(2.718));
    EXPECT_TRUE(set_value_result);
    EXPECT_EQ(result.value().kvs_.at("kvs").GetType(), KvsValue::Type::kF64);
    EXPECT_DOUBLE_EQ(std::get<double>(result.value().kvs_.at("kvs").GetValue()), 2.718);

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_key_too_long)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // A key of exactly kMaxKeyLengthBytes bytes is accepted (inclusive limit).
    std::string boundary_key(kMaxKeyLengthBytes, 'k');
    auto boundary_result = result.value().SetValue(boundary_key, KvsValue(3.14));
    EXPECT_TRUE(boundary_result);

    // A key of kMaxKeyLengthBytes + 1 bytes is rejected with kInvalidKey.
    std::string too_long_key(kMaxKeyLengthBytes + 1U, 'k');
    auto set_value_result = result.value().SetValue(too_long_key, KvsValue(3.14));
    EXPECT_FALSE(set_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kInvalidKey);

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_failure_key_too_long)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Keys are validated only when set.
    // Lookups of keys at and above kMaxKeyLengthBytes are both ordinary lookups of missing keys.
    std::string boundary_key(kMaxKeyLengthBytes, 'k');
    auto boundary_result = result.value().GetValue(boundary_key);
    EXPECT_FALSE(boundary_result);
    EXPECT_EQ(static_cast<ErrorCode>(*boundary_result.error()), ErrorCode::kKeyNotFound);

    std::string too_long_key(kMaxKeyLengthBytes + 1U, 'k');
    auto get_value_result = result.value().GetValue(too_long_key);
    EXPECT_FALSE(get_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*get_value_result.error()), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

namespace
{
/// @brief Keys with characters outside printable ASCII (0x20 - 0x7E) and a valid length.
/// @note comp_req__kvs__key_naming
std::vector<std::string> InvalidCharacterKeys()
{
    return {
        // Characters just outside the allowed range.
        "\x1f",
        "\x7f",
        // Control characters inside a key.
        "tab\tkey",
        "new\nline",
        std::string("nul\0key", 7U),
        // Valid UTF-8, but not ASCII.
        u8"ąę",
        u8"greek ημα",
        u8"emoji ✅",
        "\xc2\x80",
        // Bytes above 0x7F that are not a valid UTF-8.
        "\x80",
        "\xff",
        "ok\xc3(key",
    };
}
}  // namespace

TEST(kvs_set_value, set_value_valid_keys)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // comp_req__kvs__key_naming: any key consisting of printable ASCII characters is accepted.
    std::vector<std::string> valid_keys{"a",
                                        "Z",
                                        "0",
                                        "-",
                                        "_",
                                        "snake_case",
                                        "kebab-case",
                                        "Mixed_09-key",
                                        "with space",
                                        "dot.key",
                                        "slash/key",
                                        // Boundaries of the allowed range.
                                        " ",
                                        "~"};
    // Every allowed character on its own.
    for (char c{0x20}; c <= 0x7E; ++c)
    {
        valid_keys.emplace_back(1U, c);
    }
    for (const auto& key : valid_keys)
    {
        auto set_value_result = result.value().SetValue(key, KvsValue(3.14));
        EXPECT_TRUE(set_value_result) << "key: " << key;
        EXPECT_TRUE(result.value().kvs_.count(key)) << "key: " << key;
    }

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_empty_key)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // comp_req__kvs__key_length: an empty key is rejected with kInvalidKey.
    const auto kvs_size = result.value().kvs_.size();
    auto set_value_result = result.value().SetValue("", KvsValue(3.14));
    EXPECT_FALSE(set_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kInvalidKey);
    EXPECT_EQ(result.value().kvs_.size(), kvs_size);

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_failure_invalid_characters)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    for (const auto& key : InvalidCharacterKeys())
    {
        auto set_value_result = result.value().SetValue(key, KvsValue(3.14));
        ASSERT_FALSE(set_value_result) << "key: " << key;
        EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kInvalidKey) << "key: " << key;
        EXPECT_FALSE(result.value().kvs_.count(key)) << "key: " << key;
    }

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_failure_empty_key)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Keys are validated only when set, so an empty key is reported as missing.
    auto get_value_result = result.value().GetValue("");
    EXPECT_FALSE(get_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*get_value_result.error()), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_failure_invalid_characters)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Keys are validated only when set.
    // A key with characters outside printable ASCII is reported as missing.
    for (const auto& key : InvalidCharacterKeys())
    {
        auto get_value_result = result.value().GetValue(key);
        ASSERT_FALSE(get_value_result) << "key: " << key;
        EXPECT_EQ(static_cast<ErrorCode>(*get_value_result.error()), ErrorCode::kKeyNotFound) << "key: " << key;
    }

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_failure)
{
    PrepareEnvironment();

    // Mutex locked.
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    auto set_value_result = result.value().SetValue("new_key", KvsValue(3.0));
    EXPECT_FALSE(set_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_remove_key, remove_key_success)
{
    PrepareEnvironment();
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Check that data exists.
    EXPECT_TRUE(result.value().kvs_.count("kvs"));

    // Remove an existing key.
    auto remove_key_result = result.value().RemoveKey("kvs");
    EXPECT_TRUE(remove_key_result);
    EXPECT_FALSE(result.value().kvs_.count("kvs"));

    CleanupEnvironment();
}

TEST(kvs_remove_key, remove_key_failure)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kRequired,
                            KvsLoad::kRequired,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Remove a non-existing key.
    auto remove_key_result = result.value().RemoveKey("non_existing_key");
    EXPECT_FALSE(remove_key_result);
    EXPECT_EQ(remove_key_result.error(), ErrorCode::kKeyNotFound);

    // Mutex locked.
    result = Kvs::Open(kInstanceId,
                       kSnapshotId,
                       KvsDefaults::kRequired,
                       KvsLoad::kRequired,
                       std::string(kDataDir),
                       std::nullopt,
                       kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    remove_key_result = result.value().RemoveKey("kvs");
    EXPECT_FALSE(remove_key_result);
    EXPECT_EQ(static_cast<ErrorCode>(*remove_key_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_success)
{
    PrepareEnvironment();
    // Test writing valid JSON data, also checks GetHashBytesAdler32 and GetHashBytes.
    const char* json_test_data = R"({
        "booltest": {
            "t": "bool",
            "v": 1
        },
    })";
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    kvs->filename_prefix_ = score::filesystem::Path(kFilenamePrefix);
    auto result = kvs->WriteJsonData(json_test_data);
    EXPECT_TRUE(result);
    EXPECT_TRUE(std::filesystem::exists(kKvsPrefix + ".json"));
    EXPECT_TRUE(std::filesystem::exists(kKvsPrefix + ".hash"));

    // Check if the content and hash is correct.
    std::ifstream in_content(kKvsPrefix + ".json", std::ios::binary);
    std::string file_content((std::istreambuf_iterator<char>(in_content)), std::istreambuf_iterator<char>());
    in_content.close();
    EXPECT_EQ(file_content, json_test_data);
    std::ifstream in_hash(kKvsPrefix + ".hash", std::ios::binary);
    std::string hash_content((std::istreambuf_iterator<char>(in_hash)), std::istreambuf_iterator<char>());
    in_hash.close();
    uint32_t hash = Adler32(json_test_data);
    std::array<uint8_t, 4> hash_bytes = {uint8_t((hash >> 24) & 0xFF),
                                         uint8_t((hash >> 16) & 0xFF),
                                         uint8_t((hash >> 8) & 0xFF),
                                         uint8_t((hash) & 0xFF)};
    std::string expected_hash_string(reinterpret_cast<const char*>(hash_bytes.data()), hash_bytes.size());
    EXPECT_EQ(hash_content, expected_hash_string);

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_filesystem_failure)
{
    PrepareEnvironment();
    // Test CreateDirectories fails.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Mock filesystem.
    score::filesystem::Filesystem mock_filesystem = score::filesystem::CreateMockFileSystem();
    auto standard_mock = std::dynamic_pointer_cast<score::filesystem::StandardFilesystemMock>(mock_filesystem.standard);
    ASSERT_NE(standard_mock, nullptr);
    EXPECT_CALL(*standard_mock, CreateDirectories(::testing::_))
        .WillOnce(::testing::Return(
            score::Result<void>(score::MakeUnexpected(score::filesystem::ErrorCode::kCouldNotCreateDirectory))));
    kvs.value().filesystem_ = std::make_unique<score::filesystem::Filesystem>(std::move(mock_filesystem));

    auto result = kvs->WriteJsonData(kKvsJson);
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kPhysicalStorageFailure);

    // Test if path argument is missing parent path (will only occur if semantic errors will be done in Flush() ).
    kvs->filename_prefix_ = score::filesystem::Path("no_parent_path");
    result = kvs->WriteJsonData(kKvsJson);
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kPhysicalStorageFailure);

    CleanupEnvironment();
}
namespace
{
using score::filesystem::Perms;

/// @brief Expected permissions of KVS JSON and hash files: 0640.
constexpr Perms kExpectedFilePerms{Perms::kReadUser | Perms::kWriteUser | Perms::kReadGroup};

/// @brief JSON data different from the data stored by PrepareEnvironment().
const std::string kNewKvsJson = R"({"new_key": {"t": "i32", "v": 1}})";

score::filesystem::Filesystem RealFilesystem()
{
    return score::filesystem::FilesystemFactory{}.CreateInstance();
}

/// @brief Reads permissions of a file as an integer, readable on failure.
/// @return The permissions, or kUnknown if the status could not be retrieved.
std::uint32_t FilePerms(const score::filesystem::Path& path)
{
    const auto status = RealFilesystem().standard->Status(path);
    return static_cast<std::uint32_t>(status.has_value() ? status.value().Permissions() : Perms::kUnknown);
}

bool FileExists(const score::filesystem::Path& path)
{
    const auto exists = RealFilesystem().standard->Exists(path);
    return exists.has_value() && exists.value();
}

std::string FileContent(const score::filesystem::Path& path)
{
    std::ifstream file(path.Native(), std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

Kvs OpenTestKvs()
{
    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    EXPECT_TRUE(kvs);
    kvs->filename_prefix_ = score::filesystem::Path(kFilenamePrefix);
    return std::move(kvs.value());
}
}  // namespace

TEST(kvs_write_json_data, write_json_data_open_failure)
{
    PrepareEnvironment();
    // Test failing to open a temporary file fails the write and keeps the previous files.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const std::string previous_hash = FileContent(hash_path);
    auto kvs = OpenTestKvs();

    {
        score::os::MockGuard<::testing::NiceMock<score::os::FcntlMock>> fcntl_mock;
        EXPECT_CALL(*fcntl_mock, open(::testing::StrEq(kKvsPrefix + ".json.tmp"), ::testing::_, ::testing::_))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
        auto result = kvs.WriteJsonData(kNewKvsJson);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), ErrorCode::kPhysicalStorageFailure);
    }

    EXPECT_EQ(FileContent(json_path), kKvsJson);
    EXPECT_EQ(FileContent(hash_path), previous_hash);
    EXPECT_FALSE(FileExists(score::filesystem::Path{kKvsPrefix + ".json.tmp"}));

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_set_permissions_failure)
{
    PrepareEnvironment();
    // Test setting permissions of a written file fails and keeps the previous files.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const std::string previous_hash = FileContent(hash_path);
    auto kvs = OpenTestKvs();

    {
        score::os::MockGuard<::testing::NiceMock<score::os::StatMock>> stat_mock;
        EXPECT_CALL(*stat_mock,
                    fchmod(::testing::_,
                           score::os::Stat::Mode::kReadUser | score::os::Stat::Mode::kWriteUser |
                               score::os::Stat::Mode::kReadGroup))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EPERM))));
        auto result = kvs.WriteJsonData(kNewKvsJson);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), ErrorCode::kPhysicalStorageFailure);
    }

    EXPECT_EQ(FileContent(json_path), kKvsJson);
    EXPECT_EQ(FileContent(hash_path), previous_hash);
    EXPECT_FALSE(FileExists(score::filesystem::Path{kKvsPrefix + ".json.tmp"}));

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_staging_failure_keeps_previous_file)
{
    PrepareEnvironment();
    // Test a temporary file that cannot be removed or created fails the write and keeps the previous files.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const score::filesystem::Path json_tmp_path{kKvsPrefix + ".json.tmp"};
    const std::string previous_hash = FileContent(hash_path);
    const auto fs{RealFilesystem()};
    ASSERT_TRUE(fs.standard->CreateDirectory(json_tmp_path));
    std::ofstream{json_tmp_path.Native() + "/file"} << "data";
    auto kvs = OpenTestKvs();

    auto result = kvs.WriteJsonData(kNewKvsJson);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kPhysicalStorageFailure);
    EXPECT_EQ(FileContent(json_path), kKvsJson);
    EXPECT_EQ(FileContent(hash_path), previous_hash);
    EXPECT_FALSE(FileExists(score::filesystem::Path{kKvsPrefix + ".hash.tmp"}));

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_replaces_stale_temp_file)
{
    PrepareEnvironment();
    // Test a temporary file left behind by an interrupted write is replaced.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path json_tmp_path{kKvsPrefix + ".json.tmp"};
    const auto fs{RealFilesystem()};
    std::ofstream{json_tmp_path.Native()} << "stale";
    constexpr Perms all_perms{Perms::kReadWriteExecUser | Perms::kReadWriteExecGroup | Perms::kReadWriteExecOthers};
    ASSERT_TRUE(fs.standard->Permissions(json_tmp_path, all_perms));
    auto kvs = OpenTestKvs();

    ASSERT_TRUE(kvs.WriteJsonData(kNewKvsJson));
    EXPECT_FALSE(FileExists(json_tmp_path));
    EXPECT_EQ(FileContent(json_path), kNewKvsJson);
    EXPECT_EQ(FilePerms(json_path), static_cast<std::uint32_t>(kExpectedFilePerms));

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_leaves_no_temp_files)
{
    PrepareEnvironment();
    // Test temporary files are renamed to the JSON and hash files.
    auto kvs = OpenTestKvs();

    ASSERT_TRUE(kvs.WriteJsonData(kNewKvsJson));
    EXPECT_EQ(FileContent(score::filesystem::Path{kKvsPrefix + ".json"}), kNewKvsJson);
    EXPECT_FALSE(FileExists(score::filesystem::Path{kKvsPrefix + ".json.tmp"}));
    EXPECT_FALSE(FileExists(score::filesystem::Path{kKvsPrefix + ".hash.tmp"}));

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_permissions_ignore_umask)
{
    PrepareEnvironment();
    // Test JSON and hash files have 0640 permissions regardless of umask.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    auto kvs = OpenTestKvs();

    using Mode = score::os::Stat::Mode;
    for (const Mode umask : {Mode::kReadWriteExecGroup | Mode::kReadWriteExecOthers, Mode::kNone})
    {
        const auto previous_umask = score::os::Stat::instance().umask(umask);
        ASSERT_TRUE(previous_umask.has_value());
        const auto result = kvs.WriteJsonData(kNewKvsJson);
        score::cpp::ignore = score::os::Stat::instance().umask(previous_umask.value());

        ASSERT_TRUE(result);
        EXPECT_EQ(FilePerms(json_path), static_cast<std::uint32_t>(kExpectedFilePerms));
        EXPECT_EQ(FilePerms(hash_path), static_cast<std::uint32_t>(kExpectedFilePerms));
    }

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_sets_file_permissions)
{
    PrepareEnvironment();
    // Test newly created JSON and hash files have 0640 permissions.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const auto fs{RealFilesystem()};
    ASSERT_TRUE(fs.standard->Remove(json_path));
    ASSERT_TRUE(fs.standard->Remove(hash_path));

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    kvs->filename_prefix_ = score::filesystem::Path(kFilenamePrefix);
    ASSERT_TRUE(kvs->WriteJsonData(kKvsJson));
    EXPECT_EQ(FilePerms(json_path), static_cast<std::uint32_t>(kExpectedFilePerms));
    EXPECT_EQ(FilePerms(hash_path), static_cast<std::uint32_t>(kExpectedFilePerms));

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_overrides_existing_file_permissions)
{
    PrepareEnvironment();
    // Test existing JSON and hash files with wider permissions are narrowed to 0640.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const auto fs{RealFilesystem()};
    constexpr Perms all_perms{Perms::kReadWriteExecUser | Perms::kReadWriteExecGroup | Perms::kReadWriteExecOthers};
    ASSERT_TRUE(fs.standard->Permissions(json_path, all_perms));
    ASSERT_TRUE(fs.standard->Permissions(hash_path, all_perms));
    ASSERT_EQ(FilePerms(json_path), static_cast<std::uint32_t>(all_perms));
    ASSERT_EQ(FilePerms(hash_path), static_cast<std::uint32_t>(all_perms));

    auto kvs = Kvs::Open(InstanceId(kInstanceId),
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    kvs->filename_prefix_ = score::filesystem::Path(kFilenamePrefix);
    ASSERT_TRUE(kvs->WriteJsonData(kKvsJson));
    EXPECT_EQ(FilePerms(json_path), static_cast<std::uint32_t>(kExpectedFilePerms));
    EXPECT_EQ(FilePerms(hash_path), static_cast<std::uint32_t>(kExpectedFilePerms));

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
        EXPECT_EQ(result.value().SnapshotCount().value(), i + 1);
    }
    ASSERT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".json"));
    ASSERT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".hash"));

    // Rotate snapshots.
    auto rotate_result = result.value().SnapshotRotate();
    ASSERT_TRUE(rotate_result);

    // Check if the snapshot ids are rotated and no ID 0 exists.
    EXPECT_TRUE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".json"));
    EXPECT_TRUE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".hash"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(0) + ".json"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(0) + ".hash"));

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_max_snapshots)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
        EXPECT_EQ(result.value().SnapshotCount().value(), i + 1);
    }
    ASSERT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".json"));
    ASSERT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".hash"));

    // Check if no ID higher than kDefaultMaxSnapshots exists.
    auto rotate_result = result.value().SnapshotRotate();
    ASSERT_TRUE(rotate_result);
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots + 1) + ".json"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots + 1) + ".hash"));

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_failure_renaming_json)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
        EXPECT_EQ(result.value().SnapshotCount().value(), i + 1);
    }

    // Snapshot (JSON) renaming failed (create directories instead of JSON files to trigger a rename error).
    std::filesystem::create_directory(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".json");
    auto rotate_result = result.value().SnapshotRotate();
    EXPECT_FALSE(rotate_result);
    EXPECT_EQ(static_cast<ErrorCode>(*rotate_result.error()), ErrorCode::kPhysicalStorageFailure);

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_failure_renaming_hash)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
        EXPECT_EQ(result.value().SnapshotCount().value(), i + 1);
    }

    // Hash renaming failed (create directories instead of JSON files to trigger a rename error).
    std::filesystem::create_directory(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".hash");
    auto rotate_result = result.value().SnapshotRotate();
    EXPECT_FALSE(rotate_result);
    EXPECT_EQ(static_cast<ErrorCode>(*rotate_result.error()), ErrorCode::kPhysicalStorageFailure);

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_failure_mutex)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Mutex locked.
    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    auto rotate_result = result.value().SnapshotRotate();
    EXPECT_FALSE(rotate_result);
    EXPECT_EQ(static_cast<ErrorCode>(*rotate_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_flush, flush_success_data)
{
    PrepareEnvironment();
    // Test flush with valid data.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Clear KVS to ensure no data is written.
    result.value().kvs_.clear();
    std::string value = "value1";
    result.value().kvs_.insert({"key1", KvsValue(value)});
    auto flush_result = result.value().Flush();
    ASSERT_TRUE(flush_result);

    // Check if files were created correctly.
    EXPECT_TRUE(std::filesystem::exists(kKvsPrefix + ".json"));
    EXPECT_TRUE(std::filesystem::exists(kKvsPrefix + ".hash"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_1.json"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_1.hash"));

    CleanupEnvironment();
}

TEST(kvs_flush, flush_success_snapshot_rotate)
{
    PrepareEnvironment();

    // Test flush with valid data.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_1.json"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_1.hash"));

    // Initial flush -> snapshot ID 0.
    score::cpp::ignore = result.value().Flush();

    // Check if SnapshotRotate was triggered on second flush --> one snapshot should be available afterwards.
    auto flush_result = result.value().Flush();
    ASSERT_TRUE(flush_result);
    EXPECT_TRUE(std::filesystem::exists(kFilenamePrefix + "_1.json"));
    EXPECT_TRUE(std::filesystem::exists(kFilenamePrefix + "_1.hash"));

    CleanupEnvironment();
}

TEST(kvs_flush, flush_sets_file_permissions)
{
    PrepareEnvironment();
    // Test flush creates JSON and hash files with 0640 permissions.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const auto fs{RealFilesystem()};
    ASSERT_TRUE(fs.standard->Remove(json_path));
    ASSERT_TRUE(fs.standard->Remove(hash_path));

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    std::string value = "value1";
    result.value().kvs_.insert({"key1", KvsValue(value)});
    ASSERT_TRUE(result.value().Flush());
    EXPECT_EQ(FilePerms(json_path), static_cast<std::uint32_t>(kExpectedFilePerms));
    EXPECT_EQ(FilePerms(hash_path), static_cast<std::uint32_t>(kExpectedFilePerms));

    CleanupEnvironment();
}

TEST(kvs_flush, flush_failure_mutex)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    auto flush_result = result.value().Flush();
    EXPECT_FALSE(flush_result);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_flush, flush_failure_rotate_snapshots)
{
    // Root ignores file permissions, so flush would succeed.
    if (geteuid() == 0)
    {
        GTEST_SKIP() << "File permissions are not enforced for root.";
    }

    PrepareEnvironment();
    // Test folder for permission handling.
    std::string permissions_dir = kDataDir + "permissions/";
    std::filesystem::create_directories(permissions_dir);
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(permissions_dir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    std::filesystem::permissions(
        permissions_dir, std::filesystem::perms::owner_read, std::filesystem::perm_options::replace);
    auto flush_result = result.value().Flush();

    ASSERT_FALSE(flush_result);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_result.error()), ErrorCode::kPhysicalStorageFailure);

    CleanupEnvironment();
}

TEST(kvs_flush, flush_failure_json_writer)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Force error in writer.ToBuffer.
    auto mock_writer = std::make_unique<score::json::IJsonWriterMock>();
    EXPECT_CALL(*mock_writer, ToBuffer(::testing::A<const score::json::Object&>()))
        .WillOnce(
            ::testing::Return(score::Result<std::string>(score::MakeUnexpected(score::json::Error::kUnknownError))));

    kvs->writer_ = std::move(mock_writer);
    auto result = kvs.value().Flush();
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kJsonGeneratorError);

    CleanupEnvironment();
}

TEST(kvs_snapshot_count, snapshot_count_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        auto count = result.value().SnapshotCount();
        EXPECT_TRUE(count);
        EXPECT_EQ(count.value(), i + 1);
    }
    // Test maximum capacity.
    std::ofstream(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots + 1) + ".json") << "{}";
    auto count = result.value().SnapshotCount();
    EXPECT_TRUE(count);
    EXPECT_EQ(count.value(), kDefaultMaxSnapshots);

    CleanupEnvironment();
}

TEST(kvs_snapshot_count, snapshot_count_invalid)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Mock filesystem.
    score::filesystem::Filesystem mock_filesystem = score::filesystem::CreateMockFileSystem();
    auto standard_mock = std::dynamic_pointer_cast<score::filesystem::StandardFilesystemMock>(mock_filesystem.standard);
    ASSERT_NE(standard_mock, nullptr);
    EXPECT_CALL(*standard_mock, Exists(::testing::_))
        .WillOnce(::testing::Return(
            score::Result<bool>(score::MakeUnexpected(score::filesystem::ErrorCode::kCouldNotRetrieveStatus))));
    kvs.value().filesystem_ = std::make_unique<score::filesystem::Filesystem>(std::move(mock_filesystem));

    auto result = kvs.value().SnapshotCount();
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kPhysicalStorageFailure);
}

TEST(kvs_snapshot_restore, snapshot_restore_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    // The data received by the JSON parser should be the data listed below.
    ASSERT_FALSE(result.value().kvs_.empty());
    ASSERT_FALSE(result.value().kvs_.count("key1"));
    ASSERT_TRUE(result.value().kvs_.count("kvs"));

    // Create dummy data.
    const std::string json_data = R"({
        "kvs_old": {
            "t": "i32",
            "v": 42
        }
    })";
    // Create hash data for json_data.
    std::ofstream out(kFilenamePrefix + "_1.json", std::ios::binary);
    out.write(json_data.data(), json_data.size());
    out.close();
    uint32_t hash = Adler32(json_data);
    std::array<uint8_t, 4> hash_bytes = {uint8_t((hash >> 24) & 0xFF),
                                         uint8_t((hash >> 16) & 0xFF),
                                         uint8_t((hash >> 8) & 0xFF),
                                         uint8_t((hash) & 0xFF)};
    std::ofstream hash_out(kFilenamePrefix + "_1.hash", std::ios::binary);
    hash_out.write(reinterpret_cast<const char*>(hash_bytes.data()), hash_bytes.size());
    hash_out.close();

    auto restore_result = result.value().SnapshotRestore(1);
    EXPECT_TRUE(restore_result);
    EXPECT_TRUE(result.value().kvs_.count("kvs_old"));

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_failure_invalid_snapshot_id)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Restore snapshot ID 0 -> current KVS.
    auto restore_result = result.value().SnapshotRestore(0);
    ASSERT_FALSE(restore_result);
    EXPECT_EQ(static_cast<ErrorCode>(*restore_result.error()), ErrorCode::kInvalidSnapshotId);

    // Restore a snapshot ID higher than SnapshotCount (e.g. kDefaultMaxSnapshots + 1).
    restore_result = result.value().SnapshotRestore(kDefaultMaxSnapshots + 1);
    ASSERT_FALSE(restore_result);
    EXPECT_EQ(static_cast<ErrorCode>(*restore_result.error()), ErrorCode::kInvalidSnapshotId);

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_failure_open_json)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    // Empty JSON.
    std::ofstream(kFilenamePrefix + "_1.json") << "{}";
    // Invalid hash -> trigger OpenJson() error.
    std::ofstream(kFilenamePrefix + "_1.hash") << "invalid_hash";

    auto restore_result = result.value().SnapshotRestore(1);
    EXPECT_FALSE(restore_result);
    // Passed by OpenJson.
    EXPECT_EQ(static_cast<ErrorCode>(*restore_result.error()), ErrorCode::kValidationFailed);

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_failure_mutex)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    std::unique_lock<std::mutex> lock(result.value().kvs_mutex_);
    auto restore_result = result.value().SnapshotRestore(1);
    EXPECT_FALSE(restore_result);
    EXPECT_EQ(static_cast<ErrorCode>(*restore_result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_failure_snapshot_count)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Mock filesystem.
    score::filesystem::Filesystem mock_filesystem = score::filesystem::CreateMockFileSystem();
    auto standard_mock = std::dynamic_pointer_cast<score::filesystem::StandardFilesystemMock>(mock_filesystem.standard);
    ASSERT_NE(standard_mock, nullptr);
    EXPECT_CALL(*standard_mock, Exists(::testing::_))
        .WillOnce(::testing::Return(
            score::Result<bool>(score::MakeUnexpected(score::filesystem::ErrorCode::kCouldNotRetrieveStatus))));
    kvs.value().filesystem_ = std::make_unique<score::filesystem::Filesystem>(std::move(mock_filesystem));

    auto result = kvs.value().SnapshotRestore(1);
    EXPECT_FALSE(result);

    CleanupEnvironment();
}

TEST(kvs_snapshot_max_count, snapshot_max_count)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().SnapshotMaxCount(), kDefaultMaxSnapshots);

    CleanupEnvironment();
}

TEST(kvs_get_filename, get_kvs_filename_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Generate test files.
    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
    }

    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        auto filename = result.value().KvsFilePath(SnapshotId(i));
        EXPECT_TRUE(filename);
        EXPECT_EQ(filename.value().CStr(), kFilenamePrefix + "_" + std::to_string(i) + ".json");
    }

    CleanupEnvironment();
}

TEST(kvs_get_filename, get_kvs_filename_failure)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Test files not available.
    auto result = kvs.value().KvsFilePath(SnapshotId(1));
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kFileNotFound);

    // Filesystem exists error.
    // Mock filesystem.
    score::filesystem::Filesystem mock_filesystem = score::filesystem::CreateMockFileSystem();
    auto standard_mock = std::dynamic_pointer_cast<score::filesystem::StandardFilesystemMock>(mock_filesystem.standard);
    ASSERT_NE(standard_mock, nullptr);
    EXPECT_CALL(*standard_mock, Exists(::testing::_))
        .WillOnce(::testing::Return(
            score::Result<bool>(score::MakeUnexpected(score::filesystem::ErrorCode::kCouldNotRetrieveStatus))));
    kvs.value().filesystem_ = std::make_unique<score::filesystem::Filesystem>(std::move(mock_filesystem));

    result = kvs.value().KvsFilePath(SnapshotId(1));
    EXPECT_FALSE(result);

    CleanupEnvironment();
}

TEST(kvs_get_filename, get_hashname_success)
{
    PrepareEnvironment();

    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            kDefaultMaxSnapshots);
    ASSERT_TRUE(result);

    // Generate test files.
    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
    }

    for (size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        auto hashname = result.value().HashFilePath(SnapshotId(i));
        system("ls -l ./data_folder/");
        EXPECT_TRUE(hashname);
        EXPECT_EQ(hashname.value().CStr(), kFilenamePrefix + "_" + std::to_string(i) + ".hash");
    }

    CleanupEnvironment();
}

TEST(kvs_get_filename, get_hashname_failure)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Test files not available.
    auto result = kvs.value().HashFilePath(SnapshotId(1));
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kFileNotFound);

    // Filesystem exists error.
    // Mock filesystem.
    score::filesystem::Filesystem mock_filesystem = score::filesystem::CreateMockFileSystem();
    auto standard_mock = std::dynamic_pointer_cast<score::filesystem::StandardFilesystemMock>(mock_filesystem.standard);
    ASSERT_NE(standard_mock, nullptr);
    EXPECT_CALL(*standard_mock, Exists(::testing::_))
        .WillOnce(::testing::Return(
            score::Result<bool>(score::MakeUnexpected(score::filesystem::ErrorCode::kCouldNotRetrieveStatus))));
    kvs.value().filesystem_ = std::make_unique<score::filesystem::Filesystem>(std::move(mock_filesystem));

    result = kvs.value().HashFilePath(SnapshotId(1));
    EXPECT_FALSE(result);

    CleanupEnvironment();
}

TEST(kvs_remove_all_keys, remove_all_keys_removes_inserted_keys)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Insert some test key-value pairs.
    for (int i = 0; i < 5; i++)
    {
        auto set_result = kvs.value().SetValue("key" + std::to_string(i), KvsValue(i));
        EXPECT_TRUE(set_result);
    }

    // Remove all keys.
    auto result = kvs.value().RemoveAllKeys();
    EXPECT_TRUE(result);

    // Verify that all keys are removed.
    for (int i = 0; i < 5; i++)
    {
        auto get_result = kvs.value().GetValue("key" + std::to_string(i));
        EXPECT_FALSE(get_result);
        EXPECT_EQ(static_cast<ErrorCode>(*get_result.error()), ErrorCode::kKeyNotFound);
    }

    CleanupEnvironment();
}

TEST(kvs_remove_all_keys, remove_all_keys_empty_kvs)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    kvs.value().kvs_.clear();

    // Removing from an already empty KVS succeeds.
    auto result = kvs.value().RemoveAllKeys();
    EXPECT_TRUE(result);
    EXPECT_TRUE(kvs.value().kvs_.empty());

    CleanupEnvironment();
}

TEST(kvs_remove_all_keys, remove_all_keys_keeps_defaults)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kRequired,
                         KvsLoad::kRequired,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    kvs.value().default_values_.insert_or_assign("defaulted", KvsValue(42.0));
    auto set_result = kvs.value().SetValue("defaulted", KvsValue(7.0));
    ASSERT_TRUE(set_result);

    auto result = kvs.value().RemoveAllKeys();
    EXPECT_TRUE(result);

    // Explicit values are gone, defaults are untouched and served instead.
    EXPECT_TRUE(kvs.value().kvs_.empty());
    EXPECT_TRUE(kvs.value().default_values_.count("defaulted"));

    auto get_result = kvs.value().GetValue("defaulted");
    ASSERT_FALSE(get_result);
    EXPECT_EQ(static_cast<ErrorCode>(*get_result.error()), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_remove_all_keys, remove_all_keys_failure)
{
    PrepareEnvironment();

    // Mutex locked.
    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kRequired,
                         KvsLoad::kRequired,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    std::unique_lock<std::mutex> lock(kvs.value().kvs_mutex_);
    auto result = kvs.value().RemoveAllKeys();
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_reverts_to_state_at_open)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kRequired,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // "kvs" is loaded from the test file by PrepareEnvironment().
    ASSERT_TRUE(kvs.value().kvs_.count("kvs"));

    // Make pending changes of every kind: add, modify, remove.
    ASSERT_TRUE(kvs.value().SetValue("added", KvsValue(1.0)));
    ASSERT_TRUE(kvs.value().SetValue("kvs", KvsValue(99.0)));
    ASSERT_TRUE(kvs.value().RemoveAllKeys());

    auto result = kvs.value().DiscardPendingChanges();
    EXPECT_TRUE(result);

    // Back to what Open() loaded.
    EXPECT_FALSE(kvs.value().kvs_.count("added"));
    ASSERT_TRUE(kvs.value().kvs_.count("kvs"));
    auto get_result = kvs.value().GetValue("kvs");
    ASSERT_TRUE(get_result);
    EXPECT_EQ(get_result.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(get_result.value().GetValue()), 2);

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_reverts_to_last_flush)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value().RemoveAllKeys());
    ASSERT_TRUE(kvs.value().SetValue("kept", KvsValue(1.0)));
    ASSERT_TRUE(kvs.value().Flush());

    // Changes made after the flush are the pending ones.
    ASSERT_TRUE(kvs.value().SetValue("kept", KvsValue(2.0)));
    ASSERT_TRUE(kvs.value().SetValue("pending", KvsValue(3.0)));

    auto result = kvs.value().DiscardPendingChanges();
    EXPECT_TRUE(result);

    EXPECT_FALSE(kvs.value().kvs_.count("pending"));
    auto get_result = kvs.value().GetValue("kept");
    ASSERT_TRUE(get_result);
    EXPECT_DOUBLE_EQ(std::get<double>(get_result.value().GetValue()), 1.0);

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_without_changes_is_noop)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kRequired,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    const auto keys_before = kvs.value().kvs_.size();

    EXPECT_TRUE(kvs.value().DiscardPendingChanges());
    EXPECT_EQ(kvs.value().kvs_.size(), keys_before);

    // Repeated calls stay stable.
    EXPECT_TRUE(kvs.value().DiscardPendingChanges());
    EXPECT_EQ(kvs.value().kvs_.size(), keys_before);

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_pending_changes_failure)
{
    PrepareEnvironment();

    // Mutex locked.
    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kRequired,
                         KvsLoad::kRequired,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    std::unique_lock<std::mutex> lock(kvs.value().kvs_mutex_);
    auto result = kvs.value().DiscardPendingChanges();
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kMutexLockFailed);

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_without_persisted_file_yields_empty)
{
    PrepareEnvironment();

    // No KVS file: opened as Optional and never flushed.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value().SetValue("pending", KvsValue(1.0)));

    EXPECT_TRUE(kvs.value().DiscardPendingChanges());
    EXPECT_TRUE(kvs.value().kvs_.empty());

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_reports_corrupted_storage)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kRequired,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Corrupt the hash of the persisted file after opening.
    std::fstream corrupt_hash_file(kKvsPrefix + ".hash", std::ios::in | std::ios::out | std::ios::binary);
    corrupt_hash_file.seekp(0);
    corrupt_hash_file.put(0xFF);
    corrupt_hash_file.close();

    auto result = kvs.value().DiscardPendingChanges();
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kValidationFailed);

    CleanupEnvironment();
}

TEST(kvs_get_storage_file_size, size_matches_files_on_disk)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kRequired,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    const auto expected =
        std::filesystem::file_size(kKvsPrefix + ".json") + std::filesystem::file_size(kKvsPrefix + ".hash");

    auto result = kvs.value().GetStorageFileSize();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), expected);

    CleanupEnvironment();
}

TEST(kvs_get_storage_file_size, size_without_files_is_zero)
{
    PrepareEnvironment();

    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    auto result = kvs.value().GetStorageFileSize();
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), 0U);

    CleanupEnvironment();
}

TEST(kvs_get_storage_file_size, size_reflects_storage_not_pending_changes)
{
    PrepareEnvironment();

    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // Unflushed changes are invisible to the size query.
    ASSERT_TRUE(kvs.value().SetValue("pending", KvsValue(1.0)));
    auto before = kvs.value().GetStorageFileSize();
    ASSERT_TRUE(before);
    EXPECT_EQ(before.value(), 0U);

    ASSERT_TRUE(kvs.value().Flush());

    auto after = kvs.value().GetStorageFileSize();
    ASSERT_TRUE(after);
    EXPECT_GT(after.value(), 0U);

    CleanupEnvironment();
}

TEST(kvs_get_storage_file_size, size_failure_unreadable_path)
{
    PrepareEnvironment();

    auto kvs = Kvs::Open(kInstanceId,
                         kSnapshotId,
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional,
                         std::string(kDataDir),
                         std::nullopt,
                         kDefaultMaxSnapshots);
    ASSERT_TRUE(kvs);

    // A directory in place of the data file cannot be sized as a regular file.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    std::filesystem::create_directory(kKvsPrefix + ".json");

    auto result = kvs.value().GetStorageFileSize();
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kPhysicalStorageFailure);

    CleanupEnvironment();
}

/// @brief Storage limit used by the max-size tests.
///
/// The limit is an explicit test input: an unconfigured KVS enforces no limit at all.
constexpr size_t kTestMaxStorageBytes = 1000U;

TEST(kvs_max_storage_bytes, flush_succeeds_without_configured_limit)
{
    // The builder default is an unset optional, meaning no limit is enforced.
    // Data far larger than kTestMaxStorageBytes must therefore still flush.
    const std::string test_dir = "./kvs_no_limit_test/";
    std::filesystem::remove_all(test_dir);

    KvsBuilder builder(kInstanceId);
    builder.WorkingDir(std::string(test_dir));
    auto open_res = builder.Build();
    ASSERT_TRUE(open_res);
    Kvs kvs = std::move(open_res.value());

    const std::string large_data(kTestMaxStorageBytes * 4U, 'a');
    auto set_res = kvs.SetValue("large_data", KvsValue(large_data.c_str()));
    ASSERT_TRUE(set_res);

    auto flush_res = kvs.Flush();
    EXPECT_TRUE(flush_res);

    std::filesystem::remove_all(test_dir);
}

TEST(kvs_max_storage_bytes, flush_fails_when_storage_limit_exceeded)
{
    const std::string test_dir = "./kvs_storage_test/";
    std::filesystem::remove_all(test_dir);

    KvsBuilder builder(kInstanceId);
    builder.WorkingDir(std::string(test_dir));
    builder.MaxStorageBytes(kTestMaxStorageBytes);
    auto open_res = builder.Build();
    ASSERT_TRUE(open_res);
    Kvs kvs = std::move(open_res.value());

    // Add data close to the limit.
    // The JSON structure (key, type info, braces) and the hash file add overhead.
    // Keep the payload below the maximum.
    const size_t overhead_estimate = 100U;
    const std::string large_data(kTestMaxStorageBytes - overhead_estimate, 'a');

    auto set_res1 = kvs.SetValue("large_data", KvsValue(large_data.c_str()));
    ASSERT_TRUE(set_res1);

    // The first flush still fits.
    auto flush_res1 = kvs.Flush();
    ASSERT_TRUE(flush_res1);

    // A little more data pushes the total over the limit.
    auto set_res2 = kvs.SetValue("extra_data", KvsValue("this should not fit"));
    ASSERT_TRUE(set_res2);

    auto flush_res2 = kvs.Flush();
    ASSERT_FALSE(flush_res2);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_res2.error()), ErrorCode::kOutOfStorageSpace);

    std::filesystem::remove_all(test_dir);
}

TEST(kvs_check_size, check_size_within_limit_succeeds)
{
    const std::string test_dir = "./kvs_check_size_within_test/";
    std::filesystem::remove_all(test_dir);

    KvsBuilder builder(InstanceId(1));
    builder.WorkingDir(std::string(test_dir));
    builder.MaxStorageBytes(kTestMaxStorageBytes);
    auto open_res = builder.Build();
    ASSERT_TRUE(open_res);
    Kvs kvs = std::move(open_res.value());

    auto set_res = kvs.SetValue("key", KvsValue("some_data"));
    ASSERT_TRUE(set_res);

    auto check_res = kvs.CalculatePotentialSize();
    ASSERT_TRUE(check_res) << "CalculatePotentialSize should succeed for data within limits";
    EXPECT_GT(check_res.value(), 0U);
    EXPECT_LT(check_res.value(), kTestMaxStorageBytes);

    std::filesystem::remove_all(test_dir);
}

TEST(kvs_check_size, check_size_exceeding_limit_fails)
{
    const std::string test_dir = "./kvs_check_size_exceeding_test/";
    std::filesystem::remove_all(test_dir);

    KvsBuilder builder(InstanceId(2));
    builder.WorkingDir(std::string(test_dir));
    builder.MaxStorageBytes(kTestMaxStorageBytes);
    auto open_res = builder.Build();
    ASSERT_TRUE(open_res);
    Kvs kvs = std::move(open_res.value());

    const std::string large_data(kTestMaxStorageBytes, 'x');
    auto set_res = kvs.SetValue("oversized_key", KvsValue(large_data.c_str()));
    ASSERT_TRUE(set_res);

    auto check_res = kvs.CalculatePotentialSize();
    ASSERT_FALSE(check_res) << "CalculatePotentialSize should fail when the storage limit is exceeded";
    EXPECT_EQ(static_cast<ErrorCode>(*check_res.error()), ErrorCode::kOutOfStorageSpace);

    std::filesystem::remove_all(test_dir);
}

TEST(kvs_max_storage_bytes, flush_without_limit_does_not_measure_storage)
{
    // Without a configured limit, Flush() must not measure the persisted files at all.
    // A KVS that does not use the limit then behaves exactly as before this feature existed.
    // A directory placed where a defaults file is expected makes any size measurement fail.
    // A successful flush here proves that no measurement happened.
    const std::string test_dir = "./kvs_no_limit_no_measure_test/";
    std::filesystem::remove_all(test_dir);

    KvsBuilder builder(kInstanceId);
    builder.WorkingDir(std::string(test_dir));
    auto open_res = builder.Build();
    ASSERT_TRUE(open_res);
    Kvs kvs = std::move(open_res.value());

    // Obstruct the measurement only after opening, so Open() itself is unaffected.
    std::filesystem::create_directories(test_dir + "kvs_" + std::to_string(kInstance) + "_default.hash");

    auto set_res = kvs.SetValue("key", KvsValue("value"));
    ASSERT_TRUE(set_res);

    auto flush_res = kvs.Flush();
    EXPECT_TRUE(flush_res);

    std::filesystem::remove_all(test_dir);
}

TEST(kvs_max_storage_bytes, flush_with_limit_measures_storage)
{
    // Counterpart to flush_without_limit_does_not_measure_storage.
    // With a limit set, the same obstruction makes Flush() fail, so the obstruction really does break the measurement.
    // Without this test, the one above could pass for the wrong reason.
    // The limit is far above the data size, so the limit itself is not reached.
    const std::string test_dir = "./kvs_limit_measures_test/";
    std::filesystem::remove_all(test_dir);

    KvsBuilder builder(kInstanceId);
    builder.WorkingDir(std::string(test_dir));
    builder.MaxStorageBytes(kTestMaxStorageBytes * 1000U);
    auto open_res = builder.Build();
    ASSERT_TRUE(open_res);
    Kvs kvs = std::move(open_res.value());

    std::filesystem::create_directories(test_dir + "kvs_" + std::to_string(kInstance) + "_default.hash");

    auto set_res = kvs.SetValue("key", KvsValue("value"));
    ASSERT_TRUE(set_res);

    auto flush_res = kvs.Flush();
    ASSERT_FALSE(flush_res);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_res.error()), ErrorCode::kPhysicalStorageFailure);

    std::filesystem::remove_all(test_dir);
}

TEST(kvs_check_size, check_size_without_limit_reports_size)
{
    // CalculatePotentialSize() shares SerializeAndCheck() with Flush().
    // Skipping the measurement there when no limit is set must not stop the dry run from reporting a size.
    // Without a limit, it measures on its own and returns a real, non-zero size.
    const std::string test_dir = "./kvs_check_size_no_limit_test/";
    std::filesystem::remove_all(test_dir);

    KvsBuilder builder(InstanceId(3));
    builder.WorkingDir(std::string(test_dir));
    auto open_res = builder.Build();
    ASSERT_TRUE(open_res);
    Kvs kvs = std::move(open_res.value());

    auto set_res = kvs.SetValue("key", KvsValue("some_data"));
    ASSERT_TRUE(set_res);

    auto check_res = kvs.CalculatePotentialSize();
    ASSERT_TRUE(check_res) << "CalculatePotentialSize should succeed when no limit is configured";
    EXPECT_GT(check_res.value(), 0U);

    std::filesystem::remove_all(test_dir);
}

// comp_req__kvs__snapshot_max_num_cfg:
// "The component shall maintain a configurable maximum number of snapshots.
// The maximum number shall be in the range <0..3>."
// The tests below cover the boundary values the component integration tests exercise (0, 1, 3).
// They also cover values above the range, which are clamped.

TEST(kvs_snapshot_max_count, snapshot_max_count_configurable)
{
    PrepareEnvironment();

    // The reported maximum must be the configured one, not the compile-time default.
    for (const std::size_t configured_max : {std::size_t{0U}, std::size_t{1U}, kMaxSnapshotsUpperBound})
    {
        auto result = Kvs::Open(kInstanceId,
                                kSnapshotId,
                                KvsDefaults::kOptional,
                                KvsLoad::kOptional,
                                std::string(kDataDir),
                                std::nullopt,
                                configured_max);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value().SnapshotMaxCount(), configured_max);
    }

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_respects_configured_max)
{
    PrepareEnvironment();

    // Rotation must stop at the configured maximum.
    // A configuration that is only reported but not enforced would leave this snapshot behind.
    const std::size_t configured_max = 1U;
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            configured_max);
    ASSERT_TRUE(result);

    // Create test snapshot files up to the configured maximum.
    // Tag them, so that the rotation can be followed by content and not only by existence.
    for (std::size_t i = 0; i <= configured_max; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{\"gen\":" << i << "}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
    }

    auto rotate_result = result.value().SnapshotRotate();
    ASSERT_TRUE(rotate_result);

    // Rotation happened: the current KVS moved into the first snapshot slot and vacated slot 0.
    // Without this the test would also pass if nothing rotated.
    EXPECT_TRUE(std::filesystem::exists(kFilenamePrefix + "_1.json"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_0.json"));
    std::ifstream rotated(kFilenamePrefix + "_1.json");
    std::string rotated_content((std::istreambuf_iterator<char>(rotated)), std::istreambuf_iterator<char>());
    EXPECT_EQ(rotated_content, "{\"gen\":0}");

    // No snapshot beyond the configured maximum may be created.
    // If rotation used the compile-time default instead of the configured value, _1 would have been rotated into _2.
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(configured_max + 1U) + ".json"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(configured_max + 1U) + ".hash"));

    CleanupEnvironment();
}

TEST(kvs_snapshot_count, snapshot_count_caps_at_configured_max)
{
    PrepareEnvironment();

    // Counting must use the configured maximum as upper bound.
    // Otherwise, the count could report snapshots that rotation is not allowed to maintain.
    const std::size_t configured_max = 1U;
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            configured_max);
    ASSERT_TRUE(result);

    for (std::size_t i = 0; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
    }

    auto count = result.value().SnapshotCount();
    ASSERT_TRUE(count);
    EXPECT_EQ(count.value(), configured_max);

    CleanupEnvironment();
}

TEST(kvs_flush, flush_zero_snapshots_still_persists_current_data)
{
    PrepareEnvironment();

    // A maximum of zero snapshots means no previous generation is kept, NOT that persistency is disabled.
    // The current KVS data is still written.
    // This matches the Rust backend, whose flush() rotates (a no-op at 0) and then always saves snapshot 0.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto result = Kvs::Open(
        kInstanceId, kSnapshotId, KvsDefaults::kOptional, KvsLoad::kOptional, std::string(kDataDir), std::nullopt, 0U);
    ASSERT_TRUE(result);

    std::string value = "value1";
    result.value().kvs_.insert({"key1", KvsValue(value)});

    // The current KVS is persisted ...
    auto flush_result = result.value().Flush();
    EXPECT_TRUE(flush_result);
    EXPECT_TRUE(std::filesystem::exists(kKvsPrefix + ".json"));
    EXPECT_TRUE(std::filesystem::exists(kKvsPrefix + ".hash"));

    // ... but no previous generation is ever created ...
    auto second_flush_result = result.value().Flush();
    EXPECT_TRUE(second_flush_result);
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_1.json"));
    EXPECT_FALSE(std::filesystem::exists(kFilenamePrefix + "_1.hash"));

    // ... and no snapshot is reported.
    auto count = result.value().SnapshotCount();
    ASSERT_TRUE(count);
    EXPECT_EQ(count.value(), 0U);

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_beyond_configured_max)
{
    PrepareEnvironment();

    // Restoring a snapshot outside the configured window must be rejected, even if an older file is still present.
    const std::size_t configured_max = 1U;
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            configured_max);
    ASSERT_TRUE(result);

    for (std::size_t i = 0; i <= configured_max + 1U; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
    }

    auto restore_result = result.value().SnapshotRestore(SnapshotId(configured_max + 1U));
    ASSERT_FALSE(restore_result);
    EXPECT_EQ(static_cast<ErrorCode>(*restore_result.error()), ErrorCode::kInvalidSnapshotId);

    CleanupEnvironment();
}

TEST(kvs_constructor, move_preserves_snapshot_max_count)
{
    PrepareEnvironment();

    // The configuration is instance state and has to survive both move operations.
    // The value differs from the default, so a reset to the default would be detected.
    const std::size_t configured_max = 1U;
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            configured_max);
    ASSERT_TRUE(result);

    Kvs kvs_moved = std::move(result.value());
    EXPECT_EQ(kvs_moved.SnapshotMaxCount(), configured_max);

    auto result_default = Kvs::Open(InstanceId(kInstance + 1U),
                                    kSnapshotId,
                                    KvsDefaults::kOptional,
                                    KvsLoad::kOptional,
                                    std::string(kDataDir),
                                    std::nullopt,
                                    kDefaultMaxSnapshots);
    ASSERT_TRUE(result_default);
    Kvs kvs_assigned = std::move(result_default.value());
    ASSERT_EQ(kvs_assigned.SnapshotMaxCount(), kDefaultMaxSnapshots);

    kvs_assigned = std::move(kvs_moved);
    EXPECT_EQ(kvs_assigned.SnapshotMaxCount(), configured_max);

    CleanupEnvironment();
}

TEST(kvs_snapshot_max_count, snapshot_max_count_clamped_to_upper_bound)
{
    PrepareEnvironment();

    // comp_req__kvs__snapshot_max_num_cfg: a request above the range is clamped to kMaxSnapshotsUpperBound.
    // The first value outside the range and a far larger one must both be clamped.
    // This shows the bound is enforced for every value, not just one.
    for (const std::size_t requested : {kMaxSnapshotsUpperBound + 1U, std::size_t{100U}})
    {
        auto result = Kvs::Open(kInstanceId,
                                kSnapshotId,
                                KvsDefaults::kOptional,
                                KvsLoad::kOptional,
                                std::string(kDataDir),
                                std::nullopt,
                                requested);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value().SnapshotMaxCount(), kMaxSnapshotsUpperBound);
    }

    // Rotation follows the clamped value: no snapshot beyond the upper bound is created.
    auto result = Kvs::Open(kInstanceId,
                            kSnapshotId,
                            KvsDefaults::kOptional,
                            KvsLoad::kOptional,
                            std::string(kDataDir),
                            std::nullopt,
                            100U);
    ASSERT_TRUE(result);
    for (std::size_t i = 0; i <= kMaxSnapshotsUpperBound; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
    }
    ASSERT_TRUE(result.value().SnapshotRotate());
    EXPECT_FALSE(
        std::filesystem::exists(kFilenamePrefix + "_" + std::to_string(kMaxSnapshotsUpperBound + 1U) + ".json"));

    CleanupEnvironment();
}

TEST(kvs_max_storage_bytes, storage_size_counts_only_configured_snapshots)
{
    // The storage estimate must count only the snapshot files this instance keeps.
    // With one snapshot configured, rotation is about to overwrite an existing _1 file.
    // So it must not count against the limit.
    // Counting the default number of snapshots instead of the configured one would reject this flush although it fits.
    const std::string test_dir = "./kvs_storage_configured_snapshots_test/";
    std::filesystem::remove_all(test_dir);

    KvsBuilder builder(kInstanceId);
    builder.WorkingDir(std::string(test_dir));
    builder.SnapshotMaxCount(1U);
    builder.MaxStorageBytes(kTestMaxStorageBytes);
    auto open_res = builder.Build();
    ASSERT_TRUE(open_res);
    Kvs kvs = std::move(open_res.value());

    // A large file in snapshot slot 1, which the next rotation overwrites.
    std::filesystem::create_directories(test_dir);
    std::ofstream(test_dir + "kvs_" + std::to_string(kInstance) + "_1.json") << std::string(kTestMaxStorageBytes, 'x');

    auto set_res = kvs.SetValue("key", KvsValue("value"));
    ASSERT_TRUE(set_res);

    auto flush_res = kvs.Flush();
    EXPECT_TRUE(flush_res);

    std::filesystem::remove_all(test_dir);
}

TEST(kvs_kvsvalue, const_char_ptr_nullptr_precondition)
{
    // A null C string violates the precondition and must abort through the S-CORE assertion handler.
    const char* null_str = nullptr;
    EXPECT_DEATH(KvsValue{null_str}, "");
}

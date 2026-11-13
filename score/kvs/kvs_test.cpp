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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <utility>

// Only needed to inject a failing JSON writer: the real writer cannot fail, so this path is unreachable otherwise.
#define private public
#include "score/kvs/kvs_builder.h"
#undef private
#include "score/kvs/kvs_general_test.h"
#include "score/os/ObjectSeam.h"
#include "score/os/mocklib/fcntl_mock.h"
#include "score/os/mocklib/stat_mock.h"
#include "score/os/stat.h"
#include "score/utility.hpp"

using namespace score::kvs;

namespace
{
using score::filesystem::Perms;

/// @brief Expected permissions of KVS JSON and hash files: 0640.
constexpr Perms kExpectedFilePermissions{Perms::kReadUser | Perms::kWriteUser | Perms::kReadGroup};

/// @brief Storage limit used by the max-size tests.
///
/// The limit is an explicit test input: an unconfigured KVS enforces no limit at all.
constexpr size_t kTestMaxStorageBytes{1000U};

score::filesystem::Filesystem RealFilesystem()
{
    return score::filesystem::FilesystemFactory{}.CreateInstance();
}

/// @brief Reads permissions of a file as an integer, readable on failure.
/// @return The permissions, or kUnknown if the status could not be retrieved.
std::uint32_t FilePermissions(const score::filesystem::Path& path)
{
    const auto status{RealFilesystem().standard->Status(path)};
    return static_cast<std::uint32_t>(status.has_value() ? status.value().Permissions() : Perms::kUnknown);
}

bool FileExists(const score::filesystem::Path& path)
{
    const auto exists{RealFilesystem().standard->Exists(path)};
    return exists.has_value() && exists.value();
}

std::string FileContent(const score::filesystem::Path& path)
{
    std::ifstream file{path.Native(), std::ios::binary};
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

/// @brief Writes the defaults file and the snapshot 0 file with valid hashes, then opens the KVS.
score::Result<std::shared_ptr<Kvs>> OpenCrafted(const std::string& defaults,
                                                const std::string& kvs_data,
                                                KvsDefaults need_defaults = KvsDefaults::kRequired,
                                                KvsLoad need_kvs = KvsLoad::kRequired)
{
    PrepareEnvironment();
    Store(kDefaultPrefix + ".json", kDefaultPrefix + ".hash", defaults);
    Store(kKvsPrefix + ".json", kKvsPrefix + ".hash", kvs_data);

    return KvsBuilder{kInstanceId}
        .Snapshot(kSnapshotId)
        .Defaults(need_defaults)
        .KvsLoad(need_kvs)
        .WorkingDir(std::string(kDataDir))
        .SnapshotMaxCount(kDefaultMaxSnapshots)
        .Build();
}

/// @brief Writes the defaults file with a valid hash.
/// Call it before building the KVS, because Build() reads the defaults once.
void SeedDefaults(const std::string& json)
{
    Store(kDefaultPrefix + ".json", kDefaultPrefix + ".hash", json);
}

/// @brief Returns one i32 entry of a KVS file.
/// The key is JSON text without the quotes, so escape sequences in it stay as they are.
std::string Entry(const std::string& json_escaped_key, int value)
{
    return "\"" + json_escaped_key + "\":{\"t\":\"i32\",\"v\":" + std::to_string(value) + "}";
}

/// @brief Returns the content of a KVS file: the type-tagged root object with the given entries.
std::string KvsFile(const std::string& entries)
{
    return "{\"t\":\"obj\",\"v\":{" + entries + "}}";
}

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

/// @brief Opens the KVS with a maximum of 0 snapshots.
/// Flush() then writes snapshot 0 in place, without rotating the previous files away.
std::shared_ptr<Kvs> OpenTestKvs()
{
    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .SnapshotMaxCount(0U)
                 .Build()};

    EXPECT_TRUE(kvs);
    EXPECT_EQ((*kvs)->SnapshotMaxCount(), 0U);
    return std::move(kvs.value());
}

/// @brief Opens the KVS like OpenTestKvs() and replaces its data with "new_key".
std::shared_ptr<Kvs> OpenTestKvsWithNewKey()
{
    auto kvs{OpenTestKvs()};
    EXPECT_TRUE(kvs->RemoveAllKeys());
    EXPECT_TRUE(kvs->SetValue("new_key", KvsValue(1)));
    return kvs;
}

/// @brief Checks that the stored KVS holds only "new_key".
/// The registry is reset first, so a new KVS state loads the files and a wrong hash fails here with
/// kValidationFailed.
void ExpectOnlyNewKey()
{
    KvsInstanceRegistry::GetInstance().Reset();
    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kRequired)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(kDataDir))
                 .SnapshotMaxCount(0U)
                 .Build()};
    ASSERT_TRUE(kvs);
    auto keys{(*kvs)->GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_EQ(keys.value(), std::vector<std::string>{"new_key"});
    auto value{(*kvs)->GetValue("new_key")};
    ASSERT_TRUE(value);
    ASSERT_EQ(value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 1);
}

/// @brief Returns the JSON content that PlantGeneration() writes for snapshot `index`.
std::string GenerationTag(std::size_t index)
{
    return "{\"gen\":" + std::to_string(index) + "}";
}

/// @brief Writes the JSON and hash files of snapshot `index`, tagged with the index.
/// The hash is never read, so a tag is enough.
void PlantGeneration(std::size_t index)
{
    std::ofstream(kFilenamePrefix + "_" + std::to_string(index) + ".json") << GenerationTag(index);
    std::ofstream(kFilenamePrefix + "_" + std::to_string(index) + ".hash") << "h" << index;
}

std::string GenerationJson(std::size_t index)
{
    return FileContent(score::filesystem::Path{kFilenamePrefix + "_" + std::to_string(index) + ".json"});
}

std::string GenerationHash(std::size_t index)
{
    return FileContent(score::filesystem::Path{kFilenamePrefix + "_" + std::to_string(index) + ".hash"});
}
}  // namespace

TEST(kvs_constructor, move_constructor)
{
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_set_api, comp_req__kvs__value_get_api");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check stored and default values survive KVS move construction and assignment.");
    PrepareEnvironment();
    SeedDefaults(R"({"t": "obj", "v": {"test_default": {"t": "bool", "v": true}}})");
    const std::uint32_t instance_b{5U};

    // Create object A.
    auto result_a{KvsBuilder{instance_b}
                      .Snapshot(kSnapshotId)
                      .Defaults(KvsDefaults::kOptional)
                      .KvsLoad(KvsLoad::kOptional)
                      .WorkingDir(std::string(kDataDir))
                      .SnapshotMaxCount(kMaxSnapshotsUpperBound)
                      .Build()};
    ASSERT_TRUE(result_a);

    // Create object B.
    auto result_b{KvsBuilder{kInstanceId}
                      .Snapshot(kSnapshotId)
                      .Defaults(KvsDefaults::kOptional)
                      .KvsLoad(KvsLoad::kOptional)
                      .WorkingDir(std::string(kDataDir))
                      .SnapshotMaxCount(kMaxSnapshotsUpperBound)
                      .Build()};
    ASSERT_TRUE(result_b);
    ASSERT_TRUE(result_b.value()->SetValue("test_kvs", KvsValue(42.0)));
    KvsInstanceRegistry::GetInstance().Reset();
    Kvs kvs_a{std::move(*result_a.value())};
    Kvs kvs_b{std::move(*result_b.value())};

    auto moved_value{kvs_b.GetValue("test_kvs")};
    ASSERT_TRUE(moved_value);
    ASSERT_EQ(moved_value.value().GetType(), KvsValue::Type::kF64);
    EXPECT_EQ(std::get<double>(moved_value.value().GetValue()), 42.0);

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
    // - kvs_a now contains data from the former kvs_b.
    auto value{kvs_a.GetValue("test_kvs")};
    ASSERT_TRUE(value);
    EXPECT_EQ(value.value().GetType(), KvsValue::Type::kF64);
    EXPECT_EQ(std::get<double>(value.value().GetValue()), 42.0);

    auto loaded{kvs_a.GetValue("kvs")};
    ASSERT_TRUE(loaded);
    EXPECT_EQ(loaded.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(loaded.value().GetValue()), 2);

    auto default_value{kvs_a.GetDefaultValue("test_default")};
    ASSERT_TRUE(default_value);
    EXPECT_EQ(default_value.value().GetType(), KvsValue::Type::kBoolean);
    EXPECT_EQ(std::get<bool>(default_value.value().GetValue()), true);

    // - The file name prefix moved too, so kvs_a writes the files of the former kvs_b.
    ASSERT_TRUE(kvs_a.Flush());
    auto filename{kvs_a.KvsFilePath(SnapshotId{0ul})};
    ASSERT_TRUE(filename);
    EXPECT_EQ(filename.value().CStr(), kDataDir + "kvs_" + std::to_string(kInstance) + "_0.json");
    EXPECT_FALSE(FileExists(score::filesystem::Path{kDataDir + "kvs_" + std::to_string(instance_b) + "_0.json"}));

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_success)
{
    // Build() reads the file through OpenJson().
    // This also covers the Optional mode with an existing file.
    auto kvs{OpenCrafted(kDefaultJson,
                         R"({"t": "obj", "v": {"kvs": {"t": "i32", "v": 42}}})",
                         KvsDefaults::kOptional,
                         KvsLoad::kOptional)};
    ASSERT_TRUE(kvs);

    auto value{(*kvs)->GetValue("kvs")};
    ASSERT_TRUE(value);
    ASSERT_EQ(value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 42);

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_failure)
{
    // JSON parser failure.
    auto kvs{OpenCrafted(kDefaultJson, "{ invalid json }")};
    ASSERT_FALSE(kvs);
    EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kJsonParserError);

    // The root is valid JSON, but not a type-tagged value.
    kvs = OpenCrafted(kDefaultJson, "42");
    ASSERT_FALSE(kvs);
    EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kInvalidValueType);

    // AnyToKvsValue error.
    kvs = OpenCrafted(kDefaultJson, R"({"t": "obj", "v": {"kvs": {"t": "invalid", "v": 42}}})");
    ASSERT_FALSE(kvs);
    // Error is passed from AnyToKvsValue.
    EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kInvalidValueType);

    // Tagged root that is not an object.
    kvs = OpenCrafted(kDefaultJson, R"({"t": "i32", "v": 5})");
    ASSERT_FALSE(kvs);
    EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kJsonParserError);

    CleanupEnvironment();
}

TEST(kvs_open, storage_corrupted_all_kvs_open_modes)
{
    const KvsDefaults default_mode{KvsDefaults::kRequired};
    const InstanceId instance_id{5ul};
    const SnapshotId snapshot_id{0ul};
    const std::string default_data{R"({"t": "obj", "v": {"default": {"t": "i32","v": 10}}})"};
    const std::string snapshot_data_0{R"({"t": "obj", "v": {"kvs": {"t": "i32","v": 20}}})"};
    const std::string dir{"./kvs/"};
    const std::string prefix{"kvs_"};

    // Prepare KVS on disk.
    // Make sure files have been created.
    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0);
    ASSERT_TRUE(filesystem_test::Exists(dir + prefix + std::to_string(instance_id.id) + "_0.json"));
    ASSERT_TRUE(filesystem_test::Exists(dir + prefix + std::to_string(instance_id.id) + "_0.hash"));

    // Corrupt JSON file.
    {
        const std::string json_path{dir + prefix + std::to_string(instance_id.id) + "_0.json"};
        const std::string corrupted_data{"random_data"};
        std::ofstream file{json_path};
        if (file.is_open())
        {
            file << corrupted_data;
            file.close();
        }
    }

    // Required.
    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(snapshot_id)
                     .Defaults(default_mode)
                     .KvsLoad(KvsLoad::kRequired)
                     .WorkingDir(std::string(dir))
                     .Build()};
        ASSERT_FALSE(kvs);
        ASSERT_EQ(kvs.error(), ErrorCode::kValidationFailed);
    }

    // Optional.
    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(snapshot_id)
                     .Defaults(default_mode)
                     .KvsLoad(KvsLoad::kOptional)
                     .WorkingDir(std::string(dir))
                     .Build()};
        ASSERT_FALSE(kvs);
        ASSERT_EQ(kvs.error(), ErrorCode::kValidationFailed);
    }

    // Ignored.
    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(snapshot_id)
                     .Defaults(default_mode)
                     .KvsLoad(KvsLoad::kIgnored)
                     .WorkingDir(std::string(dir))
                     .Build()};
        // Successfully opened without loading.
        ASSERT_TRUE(kvs);

        // `kvs` should be not present.
        ASSERT_FALSE((*kvs)->GetValue("kvs"));

        // No keys available.
        auto all_keys{(*kvs)->GetAllKeys()};
        ASSERT_TRUE(all_keys);
        ASSERT_EQ(all_keys->size(), 0ul);
    }

    CleanupEnvironment(dir);
}

TEST(kvs_open, hash_missing_all_kvs_open_modes)
{
    const KvsDefaults default_mode{KvsDefaults::kRequired};
    const InstanceId instance_id{5ul};
    const SnapshotId snapshot_id{0ul};
    const std::string default_data{R"({"t": "obj", "v": {"default": {"t": "i32","v": 10}}})"};
    const std::string snapshot_data_0{R"({"t": "obj", "v": {"kvs": {"t": "i32","v": 20}}})"};
    const std::string dir{"./kvs/"};
    const std::string prefix{"kvs_"};

    // Prepare KVS on disk.
    // Make sure files have been created.
    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0);
    ASSERT_TRUE(filesystem_test::Exists(dir + prefix + std::to_string(instance_id.id) + "_0.json"));
    ASSERT_TRUE(filesystem_test::Exists(dir + prefix + std::to_string(instance_id.id) + "_0.hash"));
    // Remove hash file.
    ASSERT_TRUE(filesystem_test::Remove(dir + prefix + std::to_string(instance_id.id) + "_0.hash"));

    // Required.
    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(snapshot_id)
                     .Defaults(default_mode)
                     .KvsLoad(KvsLoad::kRequired)
                     .WorkingDir(std::string(dir))
                     .Build()};
        ASSERT_FALSE(kvs);
        ASSERT_EQ(kvs.error(), ErrorCode::kKvsHashFileReadError);
    }

    // Optional.
    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(snapshot_id)
                     .Defaults(default_mode)
                     .KvsLoad(KvsLoad::kOptional)
                     .WorkingDir(std::string(dir))
                     .Build()};
        ASSERT_FALSE(kvs);
        ASSERT_EQ(kvs.error(), ErrorCode::kKvsHashFileReadError);
    }

    // Ignored.
    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(snapshot_id)
                     .Defaults(default_mode)
                     .KvsLoad(KvsLoad::kIgnored)
                     .WorkingDir(std::string(dir))
                     .Build()};
        // Successfully opened without loading.
        ASSERT_TRUE(kvs);
        // `kvs` should be not present.
        ASSERT_FALSE((*kvs)->GetValue("kvs"));
        // No keys available.
        auto all_keys{(*kvs)->GetAllKeys()};
        ASSERT_TRUE(all_keys);
        ASSERT_EQ(all_keys->size(), 0ul);
    }

    CleanupEnvironment(dir);
}

TEST(kvs_open, selected_snapshot)
{
    const InstanceId instance_id{5ul};
    const std::string default_data{R"({"t": "obj", "v": {"default": {"t": "i32","v": 10}}})"};
    const std::string snapshot_data_0{R"({"t": "obj", "v": {"kvs": {"t": "i32","v": 20}}})"};
    const std::string snapshot_data_1{R"({"t": "obj", "v": {"kvs": {"t": "i32","v": 30}}})"};
    const std::string dir{"./kvs/"};
    const std::string prefix{"kvs_"};

    // Prepare KVS on disk with two snapshots.
    // Make sure files have been created.
    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0, snapshot_data_1);
    EXPECT_TRUE(filesystem_test::Exists(dir + prefix + std::to_string(instance_id.id) + "_0.json"));
    EXPECT_TRUE(filesystem_test::Exists(dir + prefix + std::to_string(instance_id.id) + "_1.json"));

    // First instance, load snapshot 0.
    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(SnapshotId{0ul})
                     .Defaults(KvsDefaults::kRequired)
                     .KvsLoad(KvsLoad::kRequired)
                     .WorkingDir(std::string(dir))
                     .Build()};
        ASSERT_TRUE(kvs);
        auto value{(*kvs)->GetValue("kvs")};
        ASSERT_TRUE(value);
        EXPECT_EQ(value.value().GetType(), KvsValue::Type::kI32);
        EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 20u);
    }
    CleanupEnvironment(dir);

    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0, snapshot_data_1);

    // Second instance, load snapshot 1.
    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(SnapshotId{1ul})
                     .Defaults(KvsDefaults::kRequired)
                     .KvsLoad(KvsLoad::kRequired)
                     .WorkingDir(std::string(dir))
                     .Build()};
        ASSERT_TRUE(kvs);
        auto value{(*kvs)->GetValue("kvs")};
        ASSERT_TRUE(value);
        EXPECT_EQ(value.value().GetType(), KvsValue::Type::kI32);
        EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 30u);
    }

    CleanupEnvironment(dir);
}

TEST(kvs_open, t_and_v_keys_loaded_from_file)
{
    // "t" and "v" are type tag keys only at the value level, so they are valid user keys.
    const InstanceId instance_id{5ul};
    const std::string default_data =
        R"({"t": "obj", "v": {"t": {"t": "str", "v": "default_t"}, "v": {"t": "i32", "v": 1}}})";
    const std::string snapshot_data_0 =
        R"({"t": "obj", "v": {"t": {"t": "str", "v": "obj"}, "v": {"t": "i32", "v": 5}}})";
    const std::string dir{"./kvs_t_v_keys_load/"};
    const std::string prefix{"kvs_"};

    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0);

    auto kvs{KvsBuilder{instance_id}
                 .Snapshot(SnapshotId{0ul})
                 .Defaults(KvsDefaults::kRequired)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(dir))
                 .SnapshotMaxCount(kDefaultMaxSnapshots)
                 .Build()};
    ASSERT_TRUE(kvs);

    auto keys{(*kvs)->GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_THAT(keys.value(), ::testing::UnorderedElementsAre("t", "v"));

    auto t_value{(*kvs)->GetValue("t")};
    ASSERT_TRUE(t_value);
    ASSERT_EQ(t_value.value().GetType(), KvsValue::Type::kString);
    EXPECT_EQ(std::get<std::string>(t_value.value().GetValue()), "obj");

    auto v_value{(*kvs)->GetValue("v")};
    ASSERT_TRUE(v_value);
    ASSERT_EQ(v_value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(v_value.value().GetValue()), 5);

    auto t_default{(*kvs)->GetDefaultValue("t")};
    ASSERT_TRUE(t_default);
    ASSERT_EQ(t_default.value().GetType(), KvsValue::Type::kString);
    EXPECT_EQ(std::get<std::string>(t_default.value().GetValue()), "default_t");

    auto v_default{(*kvs)->GetDefaultValue("v")};
    ASSERT_TRUE(v_default);
    ASSERT_EQ(v_default.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(v_default.value().GetValue()), 1);

    CleanupEnvironment(dir);
}
TEST(kvs_open, two_clients_the_same_instance)
{
    const InstanceId instance_id{5ul};
    const std::string default_data{R"({"t": "obj", "v": {"default": {"t": "i32","v": 10}}})"};
    const std::string snapshot_data_0{R"({"t": "obj", "v": {"kvs": {"t": "i32","v": 20}}})"};
    const std::string dir{"./two_clients_the_same_instance/"};
    const std::string prefix{"kvs_"};

    // Prepare KVS on disk with one snapshot.
    PrepareEnvironment(dir, prefix, instance_id.id, default_data, snapshot_data_0);

    // First client. The same instance.
    auto kvs1 = KvsBuilder{instance_id}
                    .Snapshot(SnapshotId{0ul})
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(dir))
                    .Build();

    // Second client. The same instance.
    auto kvs2 = KvsBuilder{instance_id}
                    .Snapshot(SnapshotId{0ul})
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(dir))
                    .Build();

    ASSERT_TRUE(kvs1);
    ASSERT_TRUE(kvs2);

    // Third client. The same instance however different params. Should fail.
    auto kvs3 = KvsBuilder{instance_id}
                    .Snapshot(SnapshotId{0ul})
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(dir))
                    .SnapshotMaxCount(1ul)
                    .Build();
    ASSERT_FALSE(kvs3);
    ASSERT_EQ(kvs3.error(), ErrorCode::kInstanceParametersMismatch);

    // Both clients see the same keys.
    auto keys1{(*kvs1)->GetAllKeys()};
    auto keys2{(*kvs2)->GetAllKeys()};
    ASSERT_TRUE(keys1);
    ASSERT_TRUE(keys2);
    EXPECT_EQ(keys1.value(), std::vector<std::string>{"kvs"});
    EXPECT_EQ(keys2.value(), keys1.value());

    // Initial value.
    auto value1{(*kvs1)->GetValue("kvs")};
    auto value2{(*kvs2)->GetValue("kvs")};
    ASSERT_TRUE(value1);
    ASSERT_TRUE(value2);
    EXPECT_EQ(std::get<int32_t>(value1.value().GetValue()), 20);
    EXPECT_EQ(std::get<int32_t>(value2.value().GetValue()), 20);

    // Change value through the first client.
    ASSERT_TRUE((*kvs1)->SetValue("kvs", KvsValue(30)));

    // Both clients see the new value.
    value1 = (*kvs1)->GetValue("kvs");
    value2 = (*kvs2)->GetValue("kvs");
    ASSERT_TRUE(value1);
    ASSERT_TRUE(value2);
    EXPECT_EQ(std::get<int32_t>(value1.value().GetValue()), 30);
    EXPECT_EQ(std::get<int32_t>(value2.value().GetValue()), 30);

    CleanupEnvironment(dir);
}

TEST(kvs_TEST, parse_json_data_rejects_over_length_key)
{
    // comp_req__kvs__key_length: an over-length key found while loading means the file is invalid or corrupted.
    // Loading must halt with an error and not silently drop data.
    const std::string valid_key(kMaxKeyLengthBytes, 'a');
    const std::string too_long_key(kMaxKeyLengthBytes + 1U, 'b');

    // A key of the maximum length loads.
    auto kvs{OpenCrafted(kDefaultJson, KvsFile(Entry(valid_key, 42)))};
    ASSERT_TRUE(kvs);
    auto exists{(*kvs)->KeyExists(valid_key)};
    ASSERT_TRUE(exists);
    EXPECT_TRUE(exists.value());

    kvs = OpenCrafted(kDefaultJson, KvsFile(Entry(valid_key, 42) + "," + Entry(too_long_key, 7)));
    ASSERT_FALSE(kvs);
    EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kInvalidKey);

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_rejects_empty_key)
{
    // comp_req__kvs__key_length: an empty key found while loading means the file is invalid or corrupted.
    // Loading must halt with an error.
    // The JSON parser already rejects an empty key, before the key check of the KVS.
    auto kvs{OpenCrafted(kDefaultJson,
                         R"({"t": "obj", "v": {"valid_key": {"t": "i32", "v": 42}, "": {"t": "i32", "v": 7}}})")};
    ASSERT_FALSE(kvs);
    EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kJsonParserError);

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_rejects_invalid_characters_key)
{
    // comp_req__kvs__key_naming: a key with characters outside printable ASCII means the file is invalid or corrupted.
    // The JSON parser rejects Unicode escapes in keys.
    // So the control and non-ASCII characters are written as raw bytes.
    // The parser accepts these keys, and the key check of the KVS rejects them.
    const std::vector<std::string> invalid_keys{"\x1f", "\x7f", "tab\\tkey", u8"ą", "\x80"};
    for (const auto& invalid_key : invalid_keys)
    {
        SCOPED_TRACE(invalid_key);
        auto kvs{OpenCrafted(kDefaultJson, KvsFile(Entry("valid_key", 42) + "," + Entry(invalid_key, 7)))};
        ASSERT_FALSE(kvs);
        EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kInvalidKey);
    }

    CleanupEnvironment();
}

TEST(kvs_TEST, parse_json_data_accepts_over_length_nested_key)
{
    // comp_req__kvs__key_length applies to root keys only.
    // Keys of nested objects are value data.
    const std::string long_key(kMaxKeyLengthBytes + 1U, 'b');
    auto kvs{OpenCrafted(kDefaultJson, KvsFile(R"("cfg":{"t":"obj","v":{)" + Entry(long_key, 7) + "}}"))};
    ASSERT_TRUE(kvs);

    auto cfg{(*kvs)->GetValue("cfg")};
    ASSERT_TRUE(cfg);
    ASSERT_EQ(cfg.value().GetType(), KvsValue::Type::kObject);
    const auto& cfg_obj{std::get<KvsValue::Object>(cfg.value().GetValue())};
    ASSERT_EQ(cfg_obj.size(), 1U);
    EXPECT_EQ(cfg_obj[0].first, long_key);
    ASSERT_EQ(cfg_obj[0].second.GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(cfg_obj[0].second.GetValue()), 7);

    CleanupEnvironment();
}

TEST(kvs_open_json, open_json_json_invalid)
{
    // An invalid defaults file fails in ParseJsonData().
    auto kvs{OpenCrafted("{ invalid json }", kKvsJson)};
    ASSERT_FALSE(kvs);
    // Error code passed by ParseJsonData().
    EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kJsonParserError);

    // A missing KVS file fails, even if its hash file exists.
    PrepareEnvironment();
    ASSERT_TRUE(RealFilesystem().standard->Remove(kKvsPrefix + ".json"));
    kvs = KvsBuilder{kInstanceId}
              .Snapshot(kSnapshotId)
              .Defaults(KvsDefaults::kRequired)
              .KvsLoad(KvsLoad::kRequired)
              .WorkingDir(std::string(kDataDir))
              .Build();
    ASSERT_FALSE(kvs);
    EXPECT_EQ(static_cast<ErrorCode>(*kvs.error()), ErrorCode::kKvsFileReadError);

    CleanupEnvironment();
}

TEST(kvs_reset, reset_success)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Check that data exists.
    auto keys_before{result.value()->GetAllKeys()};
    ASSERT_TRUE(keys_before);
    EXPECT_NE(std::find(keys_before.value().begin(), keys_before.value().end(), "kvs"), keys_before.value().end());

    // Reset KVS.
    auto reset_result{result.value()->Reset()};
    EXPECT_TRUE(reset_result);
    auto keys_after{result.value()->GetAllKeys()};
    ASSERT_TRUE(keys_after);
    EXPECT_TRUE(keys_after.value().empty());

    CleanupEnvironment();
}

TEST(kvs_get_all_keys, get_all_keys_success)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Check if all keys are returned.
    auto get_all_keys_result{result.value()->GetAllKeys()};
    ASSERT_TRUE(get_all_keys_result);
    EXPECT_FALSE(get_all_keys_result.value().empty());

    auto search{std::find(get_all_keys_result.value().begin(), get_all_keys_result.value().end(), "kvs")};
    EXPECT_TRUE(search != get_all_keys_result.value().end());

    // Check if empty keys are returned.
    ASSERT_TRUE(result.value()->RemoveAllKeys());
    get_all_keys_result = result.value()->GetAllKeys();
    EXPECT_TRUE(get_all_keys_result.value().empty());

    CleanupEnvironment();
}

TEST(kvs_key_exists, key_exists_success)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Check if key exists.
    auto exists_result{result.value()->KeyExists("kvs")};
    EXPECT_TRUE(exists_result);
    EXPECT_TRUE(exists_result.value());
    // Check if non-existing key returns false.
    exists_result = result.value()->KeyExists("non_existing_key");
    EXPECT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_success)
{
    PrepareEnvironment();
    SeedDefaults(R"({"t": "obj", "v": {"kvs": {"t": "i32", "v": 42}}})");

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Check if value is returned.
    auto get_value_result{result.value()->GetValue("kvs")};
    ASSERT_TRUE(get_value_result);
    EXPECT_EQ(get_value_result.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(get_value_result.value().GetValue()), 2);

    // GetValue() does not fall back to the default value of a removed key.
    ASSERT_TRUE(result.value()->RemoveKey("kvs"));
    auto get_default_result{result.value()->GetDefaultValue("kvs")};
    ASSERT_TRUE(get_default_result);
    EXPECT_EQ(std::get<int32_t>(get_default_result.value().GetValue()), 42);
    get_value_result = result.value()->GetValue("kvs");
    ASSERT_FALSE(get_value_result);
    EXPECT_EQ(get_value_result.error(), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_failure)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Check if non-existing key returns error.
    auto get_value_result{result.value()->GetValue("non_existing_key")};
    EXPECT_FALSE(get_value_result);
    EXPECT_EQ(get_value_result.error(), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_get_default_value, get_default_value_success)
{
    PrepareEnvironment();
    SeedDefaults(R"({"t": "obj", "v": {"kvs": {"t": "i32", "v": 42}}})");

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Check if default value is returned.
    auto get_default_value_result{result.value()->GetDefaultValue("kvs")};
    ASSERT_TRUE(get_default_value_result);
    EXPECT_EQ(get_default_value_result.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(get_default_value_result.value().GetValue()), 42);

    // The written value differs from the default value.
    auto get_value_result{result.value()->GetValue("kvs")};
    ASSERT_TRUE(get_value_result);
    EXPECT_EQ(std::get<int32_t>(get_value_result.value().GetValue()), 2);

    CleanupEnvironment();
}

TEST(kvs_get_default_value, get_default_value_failure)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Check if non-existing key returns error.
    auto get_default_value_result{result.value()->GetDefaultValue("non_existing_key")};
    EXPECT_FALSE(get_default_value_result);
    EXPECT_EQ(get_default_value_result.error(), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_set_value_to_default, set_value_to_default_success)
{
    PrepareEnvironment();
    SeedDefaults(R"({"t": "obj", "v": {"default": {"t": "i32", "v": 5}, "kvs": {"t": "f64", "v": 42.0}}})");

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);
    // Check that data exists.
    auto exists_result{result.value()->KeyExists("kvs")};
    ASSERT_TRUE(exists_result);
    ASSERT_TRUE(exists_result.value());
    // Set a written key to its default value.
    auto set_value_to_default_result{result.value()->SetValueToDefault("kvs")};
    EXPECT_TRUE(set_value_to_default_result);
    exists_result = result.value()->KeyExists("kvs");
    ASSERT_TRUE(exists_result);
    EXPECT_TRUE(exists_result.value());
    auto value_result{result.value()->GetValue("kvs")};
    ASSERT_TRUE(value_result);
    ASSERT_EQ(value_result.value().GetType(), KvsValue::Type::kF64);
    EXPECT_EQ(std::get<double>(value_result.value().GetValue()), 42.0);

    // Set a non written key which has default value.
    exists_result = result.value()->KeyExists("default");
    ASSERT_TRUE(exists_result);
    ASSERT_FALSE(exists_result.value());
    set_value_to_default_result = result.value()->SetValueToDefault("default");
    EXPECT_TRUE(set_value_to_default_result);
    exists_result = result.value()->KeyExists("default");
    ASSERT_TRUE(exists_result);
    EXPECT_TRUE(exists_result.value());
    value_result = result.value()->GetValue("default");
    ASSERT_TRUE(value_result);
    ASSERT_EQ(value_result.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(value_result.value().GetValue()), 5);

    CleanupEnvironment();
}

TEST(kvs_set_value_to_default, set_value_to_default_failure)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(kDefaultMaxSnapshots)
                    .Build()};

    ASSERT_TRUE(result);

    // Set a non-existing key to default.
    auto set_value_to_default_result{result.value()->SetValueToDefault("non_existing_key")};
    EXPECT_FALSE(set_value_to_default_result);
    EXPECT_EQ(set_value_to_default_result.error(), ErrorCode::kKeyNotFound);

    // Set a key without default value.
    result = KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kRequired)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(kDataDir))
                 .SnapshotMaxCount(kDefaultMaxSnapshots)
                 .Build();
    ASSERT_TRUE(result);
    // default_json has no "kvs" key, so "kvs" has no default value.
    auto get_default_result{result.value()->GetDefaultValue("kvs")};
    ASSERT_FALSE(get_default_result);
    EXPECT_EQ(get_default_result.error(), ErrorCode::kKeyNotFound);
    set_value_to_default_result = result.value()->SetValueToDefault("kvs");
    EXPECT_FALSE(set_value_to_default_result);
    EXPECT_EQ(set_value_to_default_result.error(), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_set_value_to_default, set_value_to_default_failure_invalid_key)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(kDefaultMaxSnapshots)
                    .Build()};
    ASSERT_TRUE(result);

    // Keys are validated before the default value lookup.
    // None of these keys has a default value, so a lookup first would return kKeyNotFound.
    std::vector<std::string> invalid_keys{InvalidCharacterKeys()};
    invalid_keys.emplace_back("");
    invalid_keys.emplace_back(kMaxKeyLengthBytes + 1U, 'k');
    const auto keys_before{result.value()->GetAllKeys()};
    ASSERT_TRUE(keys_before);
    for (const auto& key : invalid_keys)
    {
        auto set_value_to_default_result{result.value()->SetValueToDefault(key)};
        ASSERT_FALSE(set_value_to_default_result) << "key: " << key;
        EXPECT_EQ(static_cast<ErrorCode>(*set_value_to_default_result.error()), ErrorCode::kInvalidKey)
            << "key: " << key;
    }
    const auto keys_after{result.value()->GetAllKeys()};
    ASSERT_TRUE(keys_after);
    EXPECT_EQ(keys_after.value(), keys_before.value());

    CleanupEnvironment();
}

TEST(kvs_is_value_default, is_value_default)
{
    PrepareEnvironment();
    SeedDefaults(R"({"t": "obj", "v": {"default": {"t": "f64", "v": 42.0}}})");

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);
    auto kvs{std::move(result.value())};

    // The seeded default replaces the "default" key of default_json.
    auto default_result{kvs->GetDefaultValue("default")};
    ASSERT_TRUE(default_result);
    ASSERT_EQ(default_result.value().GetType(), KvsValue::Type::kF64);
    EXPECT_DOUBLE_EQ(std::get<double>(default_result.value().GetValue()), 42.0);

    // Write a value that has no default value.
    ASSERT_TRUE(kvs->SetValue("not-default", KvsValue(123.4)));

    // Check non-default value.
    {
        auto not_default_result{kvs->IsValueDefault("not-default")};
        ASSERT_TRUE(not_default_result);
        ASSERT_FALSE(not_default_result.value());
    }

    // Check default value.
    {
        auto default_result{kvs->IsValueDefault("default")};
        ASSERT_TRUE(default_result);
        ASSERT_TRUE(default_result.value());
    }

    // Check key not found.
    {
        auto missing_result{kvs->IsValueDefault("non_existing_key")};
        ASSERT_FALSE(missing_result);
        ASSERT_EQ(missing_result.error(), ErrorCode::kKeyNotFound);
    }

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_success)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Set a new value.
    auto set_value_result{result.value()->SetValue("new_key", KvsValue(3.14))};
    EXPECT_TRUE(set_value_result);
    auto exists_result{result.value()->KeyExists("new_key")};
    ASSERT_TRUE(exists_result);
    EXPECT_TRUE(exists_result.value());
    auto get_value_result{result.value()->GetValue("new_key")};
    ASSERT_TRUE(get_value_result);
    EXPECT_EQ(get_value_result.value().GetType(), KvsValue::Type::kF64);
    EXPECT_DOUBLE_EQ(std::get<double>(get_value_result.value().GetValue()), 3.14);

    // Set a value with an existing key.
    set_value_result = result.value()->SetValue("kvs", KvsValue(2.718));
    EXPECT_TRUE(set_value_result);
    get_value_result = result.value()->GetValue("kvs");
    ASSERT_TRUE(get_value_result);
    EXPECT_EQ(get_value_result.value().GetType(), KvsValue::Type::kF64);
    EXPECT_DOUBLE_EQ(std::get<double>(get_value_result.value().GetValue()), 2.718);

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_key_too_long)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // A key of exactly kMaxKeyLengthBytes bytes is accepted (inclusive limit).
    std::string boundary_key(kMaxKeyLengthBytes, 'k');
    auto boundary_result{result.value()->SetValue(boundary_key, KvsValue(3.14))};
    EXPECT_TRUE(boundary_result);

    // A key of kMaxKeyLengthBytes + 1 bytes is rejected with kInvalidKey.
    std::string too_long_key(kMaxKeyLengthBytes + 1U, 'k');
    auto set_value_result{result.value()->SetValue(too_long_key, KvsValue(3.14))};
    EXPECT_FALSE(set_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kInvalidKey);

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_failure_key_too_long)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Keys are validated only when set.
    // Lookups of keys at and above kMaxKeyLengthBytes are both ordinary lookups of missing keys.
    std::string boundary_key(kMaxKeyLengthBytes, 'k');
    auto boundary_result{result.value()->GetValue(boundary_key)};
    EXPECT_FALSE(boundary_result);
    EXPECT_EQ(static_cast<ErrorCode>(*boundary_result.error()), ErrorCode::kKeyNotFound);

    std::string too_long_key(kMaxKeyLengthBytes + 1U, 'k');
    auto get_value_result{result.value()->GetValue(too_long_key)};
    EXPECT_FALSE(get_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*get_value_result.error()), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_valid_keys)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
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
        SCOPED_TRACE(key);
        auto set_value_result{result.value()->SetValue(key, KvsValue(3.14))};
        EXPECT_TRUE(set_value_result);
        auto exists_result{result.value()->KeyExists(key)};
        ASSERT_TRUE(exists_result);
        EXPECT_TRUE(exists_result.value());
    }

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_empty_key)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // comp_req__kvs__key_length: an empty key is rejected with kInvalidKey.
    const auto keys_before{result.value()->GetAllKeys()};
    ASSERT_TRUE(keys_before);
    auto set_value_result{result.value()->SetValue("", KvsValue(3.14))};
    EXPECT_FALSE(set_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kInvalidKey);
    const auto keys_after{result.value()->GetAllKeys()};
    ASSERT_TRUE(keys_after);
    EXPECT_EQ(keys_after.value().size(), keys_before.value().size());

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_failure_invalid_characters)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    for (const auto& key : InvalidCharacterKeys())
    {
        auto set_value_result{result.value()->SetValue(key, KvsValue(3.14))};
        ASSERT_FALSE(set_value_result) << "key: " << key;
        EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kInvalidKey) << "key: " << key;
        auto exists_result{result.value()->KeyExists(key)};
        ASSERT_TRUE(exists_result) << "key: " << key;
        EXPECT_FALSE(exists_result.value()) << "key: " << key;
    }

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_failure_empty_key)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Keys are validated only when set, so an empty key is reported as missing.
    auto get_value_result{result.value()->GetValue("")};
    EXPECT_FALSE(get_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*get_value_result.error()), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_get_value, get_value_failure_invalid_characters)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Keys are validated only when set.
    // A key with characters outside printable ASCII is reported as missing.
    for (const auto& key : InvalidCharacterKeys())
    {
        auto get_value_result{result.value()->GetValue(key)};
        ASSERT_FALSE(get_value_result) << "key: " << key;
        EXPECT_EQ(static_cast<ErrorCode>(*get_value_result.error()), ErrorCode::kKeyNotFound) << "key: " << key;
    }

    CleanupEnvironment();
}

TEST(kvs_set_value, set_value_duplicate_object_key)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(kDefaultMaxSnapshots)
                    .Build()};
    ASSERT_TRUE(result);

    // Duplicate key in a top-level object.
    KvsValue::Object flat{{"a", KvsValue(int32_t{1})}, {"a", KvsValue(int32_t{2})}};
    auto set_value_result{result.value()->SetValue("flat", KvsValue(std::move(flat)))};
    EXPECT_FALSE(set_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kInvalidValueType);
    auto exists_result{result.value()->KeyExists("flat")};
    ASSERT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());

    // Duplicate key in an object nested in an array.
    KvsValue::Object inner{{"b", KvsValue(true)}, {"b", KvsValue(false)}};
    KvsValue::Array array{KvsValue(int32_t{0}), KvsValue(std::move(inner))};
    KvsValue::Object outer{{"array", KvsValue(std::move(array))}};
    set_value_result = result.value()->SetValue("nested", KvsValue(std::move(outer)));
    EXPECT_FALSE(set_value_result);
    EXPECT_EQ(static_cast<ErrorCode>(*set_value_result.error()), ErrorCode::kInvalidValueType);
    exists_result = result.value()->KeyExists("nested");
    ASSERT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());

    CleanupEnvironment();
}

TEST(kvs_set_value, t_and_v_keys_round_trip)
{
    // "t" and "v" set by the user must survive Flush and reload.
    const InstanceId instance_id{5ul};
    const std::string dir{"./kvs_t_v_keys_set/"};
    const std::string prefix{"kvs_"};

    PrepareEnvironment(dir, prefix, instance_id.id, R"({"t": "obj", "v": {}})");

    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(SnapshotId{0ul})
                     .Defaults(KvsDefaults::kRequired)
                     .KvsLoad(KvsLoad::kOptional)
                     .WorkingDir(std::string(dir))
                     .Build()};
        ASSERT_TRUE(kvs);

        EXPECT_TRUE((*kvs)->SetValue("t", KvsValue("obj")));
        EXPECT_TRUE((*kvs)->SetValue("v", KvsValue(int32_t{42})));
        KvsValue::Object nested{{"t", KvsValue("x")}, {"v", KvsValue(int32_t{7})}};
        EXPECT_TRUE((*kvs)->SetValue("nested", KvsValue(std::move(nested))));
        ASSERT_TRUE((*kvs)->Flush());
    }

    // Drop the shared state so the next build reloads the flushed files.
    KvsInstanceRegistry::GetInstance().Reset();
    auto kvs{KvsBuilder{instance_id}
                 .Snapshot(SnapshotId{0ul})
                 .Defaults(KvsDefaults::kRequired)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(dir))
                 .Build()};
    ASSERT_TRUE(kvs);

    auto t_value{(*kvs)->GetValue("t")};
    ASSERT_TRUE(t_value);
    ASSERT_EQ(t_value.value().GetType(), KvsValue::Type::kString);
    EXPECT_EQ(std::get<std::string>(t_value.value().GetValue()), "obj");

    auto v_value{(*kvs)->GetValue("v")};
    ASSERT_TRUE(v_value);
    ASSERT_EQ(v_value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(v_value.value().GetValue()), 42);

    auto nested_value{(*kvs)->GetValue("nested")};
    ASSERT_TRUE(nested_value);
    ASSERT_EQ(nested_value.value().GetType(), KvsValue::Type::kObject);
    const auto& nested_obj{std::get<KvsValue::Object>(nested_value.value().GetValue())};
    ASSERT_EQ(nested_obj.size(), 2U);
    EXPECT_EQ(nested_obj[0].first, "t");
    ASSERT_EQ(nested_obj[0].second.GetType(), KvsValue::Type::kString);
    EXPECT_EQ(std::get<std::string>(nested_obj[0].second.GetValue()), "x");
    EXPECT_EQ(nested_obj[1].first, "v");
    ASSERT_EQ(nested_obj[1].second.GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(nested_obj[1].second.GetValue()), 7);

    CleanupEnvironment(dir);
}

TEST(kvs_set_value, over_length_nested_key_round_trip)
{
    // comp_req__kvs__key_length applies to root keys only.
    // A nested object with an over-length key must survive Flush and reload.
    const InstanceId instance_id{5ul};
    const std::string dir{"./kvs_long_nested_key/"};
    const std::string prefix{"kvs_"};
    const std::string long_key(kMaxKeyLengthBytes + 1U, 'x');

    PrepareEnvironment(dir, prefix, instance_id.id, R"({"t": "obj", "v": {}})");

    {
        auto kvs{KvsBuilder{instance_id}
                     .Snapshot(SnapshotId{0ul})
                     .Defaults(KvsDefaults::kRequired)
                     .KvsLoad(KvsLoad::kOptional)
                     .WorkingDir(std::string(dir))
                     .Build()};
        ASSERT_TRUE(kvs);

        KvsValue::Object cfg{{long_key, KvsValue(int32_t{1})}};
        EXPECT_TRUE((*kvs)->SetValue("cfg", KvsValue(std::move(cfg))));
        ASSERT_TRUE((*kvs)->Flush());
    }

    // Drop the shared state so the next build reloads the flushed files.
    KvsInstanceRegistry::GetInstance().Reset();
    auto kvs{KvsBuilder{instance_id}
                 .Snapshot(SnapshotId{0ul})
                 .Defaults(KvsDefaults::kRequired)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(dir))
                 .Build()};
    ASSERT_TRUE(kvs);

    auto cfg_value{(*kvs)->GetValue("cfg")};
    ASSERT_TRUE(cfg_value);
    ASSERT_EQ(cfg_value.value().GetType(), KvsValue::Type::kObject);
    const auto& cfg_obj{std::get<KvsValue::Object>(cfg_value.value().GetValue())};
    ASSERT_EQ(cfg_obj.size(), 1U);
    EXPECT_EQ(cfg_obj[0].first, long_key);
    ASSERT_EQ(cfg_obj[0].second.GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(cfg_obj[0].second.GetValue()), 1);

    CleanupEnvironment(dir);
}

TEST(kvs_remove_key, remove_key_success)
{
    PrepareEnvironment();
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Check that data exists.
    auto exists_result{result.value()->KeyExists("kvs")};
    ASSERT_TRUE(exists_result);
    EXPECT_TRUE(exists_result.value());

    // Remove an existing key.
    auto remove_key_result{result.value()->RemoveKey("kvs")};
    EXPECT_TRUE(remove_key_result);
    exists_result = result.value()->KeyExists("kvs");
    ASSERT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());

    CleanupEnvironment();
}

TEST(kvs_remove_key, remove_key_failure)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kRequired)
                    .KvsLoad(KvsLoad::kRequired)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Remove a non-existing key.
    auto remove_key_result{result.value()->RemoveKey("non_existing_key")};
    EXPECT_FALSE(remove_key_result);
    EXPECT_EQ(remove_key_result.error(), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_success)
{
    PrepareEnvironment();
    // Test Flush() writes the JSON file and its hash.
    // This also checks GetHashBytesAdler32 and GetHashBytes.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .SnapshotMaxCount(0U)
                 .Build()};
    ASSERT_TRUE(kvs);

    ASSERT_TRUE((*kvs)->SetValue("booltest", KvsValue(true)));
    ASSERT_TRUE((*kvs)->Flush());
    EXPECT_TRUE(filesystem_test::Exists(kKvsPrefix + ".json"));
    EXPECT_TRUE(filesystem_test::Exists(kKvsPrefix + ".hash"));

    // The hash file holds the big-endian Adler-32 of the JSON file.
    const std::string json_content{FileContent(score::filesystem::Path{kKvsPrefix + ".json"})};
    const std::string hash_content{FileContent(score::filesystem::Path{kKvsPrefix + ".hash"})};
    uint32_t hash{Adler32(json_content)};
    std::array<uint8_t, 4> hash_bytes{uint8_t((hash >> 24) & 0xFF),
                                      uint8_t((hash >> 16) & 0xFF),
                                      uint8_t((hash >> 8) & 0xFF),
                                      uint8_t((hash) & 0xFF)};
    std::string expected_hash_string{reinterpret_cast<const char*>(hash_bytes.data()), hash_bytes.size()};
    EXPECT_EQ(hash_content, expected_hash_string);

    KvsInstanceRegistry::GetInstance().Reset();

    // A second KVS loads the written data.
    auto reopened{KvsBuilder{kInstanceId}
                      .Snapshot(kSnapshotId)
                      .Defaults(KvsDefaults::kRequired)
                      .KvsLoad(KvsLoad::kRequired)
                      .WorkingDir(std::string(kDataDir))
                      .SnapshotMaxCount(0ul)
                      .Build()};
    ASSERT_TRUE(reopened);
    auto value{(*reopened)->GetValue("booltest")};
    ASSERT_TRUE(value);
    ASSERT_EQ(value.value().GetType(), KvsValue::Type::kBoolean);
    EXPECT_TRUE(std::get<bool>(value.value().GetValue()));

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_filesystem_failure)
{
    PrepareEnvironment();
    // Test CreateDirectories fails and keeps the previous files.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const std::string previous_hash{FileContent(hash_path)};
    auto kvs_handle{OpenTestKvsWithNewKey()};
    auto& kvs{*kvs_handle};

    {
        score::os::MockGuard<::testing::NiceMock<score::os::StatMock>> stat_mock;
        EXPECT_CALL(*stat_mock, mkdir(::testing::_, ::testing::_))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
        auto result{kvs.Flush()};
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error(), ErrorCode::kPhysicalStorageFailure);
    }

    EXPECT_EQ(FileContent(json_path), kKvsJson);
    EXPECT_EQ(FileContent(hash_path), previous_hash);

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_open_failure)
{
    PrepareEnvironment();
    // Test failing to open a temporary file fails the write and keeps the previous files.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const std::string previous_hash{FileContent(hash_path)};
    auto kvs_handle{OpenTestKvsWithNewKey()};
    auto& kvs{*kvs_handle};

    {
        score::os::MockGuard<::testing::NiceMock<score::os::FcntlMock>> fcntl_mock;
        EXPECT_CALL(*fcntl_mock, open(::testing::StrEq(kKvsPrefix + ".json.tmp"), ::testing::_, ::testing::_))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
        auto result{kvs.Flush()};
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
    const std::string previous_hash{FileContent(hash_path)};
    auto kvs_handle{OpenTestKvsWithNewKey()};
    auto& kvs{*kvs_handle};

    {
        score::os::MockGuard<::testing::NiceMock<score::os::StatMock>> stat_mock;
        EXPECT_CALL(*stat_mock,
                    fchmod(::testing::_,
                           score::os::Stat::Mode::kReadUser | score::os::Stat::Mode::kWriteUser |
                               score::os::Stat::Mode::kReadGroup))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EPERM))));
        auto result{kvs.Flush()};
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
    const std::string previous_hash{FileContent(hash_path)};
    const auto fs{RealFilesystem()};
    ASSERT_TRUE(fs.standard->CreateDirectory(json_tmp_path));
    std::ofstream{json_tmp_path.Native() + "/file"} << "data";
    auto kvs_handle{OpenTestKvsWithNewKey()};
    auto& kvs{*kvs_handle};

    auto result{kvs.Flush()};
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
    constexpr Perms all_permissions{Perms::kReadWriteExecUser | Perms::kReadWriteExecGroup |
                                    Perms::kReadWriteExecOthers};
    ASSERT_TRUE(fs.standard->Permissions(json_tmp_path, all_permissions));
    auto kvs_handle{OpenTestKvsWithNewKey()};
    auto& kvs{*kvs_handle};

    ASSERT_TRUE(kvs.Flush());
    EXPECT_FALSE(FileExists(json_tmp_path));
    ExpectOnlyNewKey();
    EXPECT_EQ(FilePermissions(json_path), static_cast<std::uint32_t>(kExpectedFilePermissions));

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_leaves_no_temp_files)
{
    PrepareEnvironment();
    // Test temporary files are renamed to the JSON and hash files.
    auto kvs_handle{OpenTestKvsWithNewKey()};
    auto& kvs{*kvs_handle};

    ASSERT_TRUE(kvs.Flush());
    ExpectOnlyNewKey();
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
    auto kvs{OpenTestKvs()};

    using Mode = score::os::Stat::Mode;
    for (const Mode umask : {Mode::kReadWriteExecGroup | Mode::kReadWriteExecOthers, Mode::kNone})
    {
        const auto previous_umask{score::os::Stat::instance().umask(umask)};
        ASSERT_TRUE(previous_umask.has_value());
        const auto result{kvs->Flush()};
        score::cpp::ignore = score::os::Stat::instance().umask(previous_umask.value());

        ASSERT_TRUE(result);
        EXPECT_EQ(FilePermissions(json_path), static_cast<std::uint32_t>(kExpectedFilePermissions));
        EXPECT_EQ(FilePermissions(hash_path), static_cast<std::uint32_t>(kExpectedFilePermissions));
    }

    CleanupEnvironment();
}

TEST(kvs_write_json_data, write_json_data_overrides_existing_file_permissions)
{
    PrepareEnvironment();
    // Test existing JSON and hash files with wider permissions are narrowed to 0640.
    const score::filesystem::Path json_path{kKvsPrefix + ".json"};
    const score::filesystem::Path hash_path{kKvsPrefix + ".hash"};
    const auto fs{RealFilesystem()};
    constexpr Perms all_permissions{Perms::kReadWriteExecUser | Perms::kReadWriteExecGroup |
                                    Perms::kReadWriteExecOthers};
    ASSERT_TRUE(fs.standard->Permissions(json_path, all_permissions));
    ASSERT_TRUE(fs.standard->Permissions(hash_path, all_permissions));
    ASSERT_EQ(FilePermissions(json_path), static_cast<std::uint32_t>(all_permissions));
    ASSERT_EQ(FilePermissions(hash_path), static_cast<std::uint32_t>(all_permissions));

    // With a maximum of 0 snapshots, Flush() replaces these files instead of rotating them to snapshot 1.
    auto kvs_handle{OpenTestKvsWithNewKey()};
    auto& kvs{*kvs_handle};
    ASSERT_TRUE(kvs.Flush());
    EXPECT_EQ(FilePermissions(json_path), static_cast<std::uint32_t>(kExpectedFilePermissions));
    EXPECT_EQ(FilePermissions(hash_path), static_cast<std::uint32_t>(kExpectedFilePermissions));

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_success)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Create test snapshot files, tagged with their index.
    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        PlantGeneration(i);
        EXPECT_EQ(result.value()->SnapshotCount().value(), i + 1);
    }
    ASSERT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".json"));
    ASSERT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".hash"));

    // Flush() rotates the snapshots, then writes a new snapshot 0.
    ASSERT_TRUE(result.value()->Flush());

    // Every snapshot moved one ID up, so the last ID holds the previous last snapshot.
    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        SCOPED_TRACE(i);
        EXPECT_EQ(GenerationJson(i + 1), GenerationTag(i));
        EXPECT_EQ(GenerationHash(i + 1), "h" + std::to_string(i));
    }

    // Snapshot 0 holds the new data with a valid hash.
    EXPECT_NE(GenerationJson(0), GenerationTag(0));
    KvsInstanceRegistry::GetInstance().Reset();
    auto reopened{KvsBuilder{kInstanceId}
                      .Snapshot(kSnapshotId)
                      .Defaults(KvsDefaults::kOptional)
                      .KvsLoad(KvsLoad::kRequired)
                      .WorkingDir(std::string(kDataDir))
                      .Build()};
    EXPECT_TRUE(reopened);

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_max_snapshots)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Create test snapshot files, tagged with their index.
    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        PlantGeneration(i);
        EXPECT_EQ(result.value()->SnapshotCount().value(), i + 1);
    }
    // Also fill the last ID, so that a rotation one step too far has a file to move.
    PlantGeneration(kDefaultMaxSnapshots);

    // Check if no ID higher than kDefaultMaxSnapshots exists.
    ASSERT_TRUE(result.value()->Flush());
    EXPECT_EQ(GenerationJson(kDefaultMaxSnapshots), GenerationTag(kDefaultMaxSnapshots - 1U));
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots + 1) + ".json"));
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots + 1) + ".hash"));

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_failure_renaming_json)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Create test snapshot files, tagged with their index.
    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        PlantGeneration(i);
        EXPECT_EQ(result.value()->SnapshotCount().value(), i + 1);
    }

    // Snapshot (JSON) renaming failed (create directories instead of JSON files to trigger a rename error).
    ASSERT_TRUE(RealFilesystem().standard->CreateDirectory(
        score::filesystem::Path{kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".json"}));
    auto flush_result{result.value()->Flush()};
    ASSERT_FALSE(flush_result);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_result.error()), ErrorCode::kPhysicalStorageFailure);
    // The rotation failed, so Flush() did not write a new snapshot 0.
    EXPECT_EQ(GenerationJson(0), GenerationTag(0));

    CleanupEnvironment();
}

TEST(kvs_snapshot_rotate, snapshot_rotate_failure_renaming_hash)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};

    ASSERT_TRUE(result);

    // Create test snapshot files, tagged with their index.
    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        PlantGeneration(i);
        EXPECT_EQ(result.value()->SnapshotCount().value(), i + 1);
    }

    // Hash renaming failed (create directories instead of JSON files to trigger a rename error).
    ASSERT_TRUE(RealFilesystem().standard->CreateDirectory(
        score::filesystem::Path{kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots) + ".hash"}));
    auto flush_result{result.value()->Flush()};
    ASSERT_FALSE(flush_result);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_result.error()), ErrorCode::kPhysicalStorageFailure);
    // The rotation failed, so Flush() did not write a new snapshot 0.
    EXPECT_EQ(GenerationJson(0), GenerationTag(0));

    CleanupEnvironment();
}

TEST(kvs_flush, flush_success_data)
{
    PrepareEnvironment();
    // Test flush with valid data.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Clear KVS to ensure no data is written.
    ASSERT_TRUE(result.value()->RemoveAllKeys());
    std::string value{"value1"};
    ASSERT_TRUE(result.value()->SetValue("key1", KvsValue(value)));
    auto flush_result{result.value()->Flush()};
    ASSERT_TRUE(flush_result);

    // Check if files were created correctly.
    EXPECT_TRUE(filesystem_test::Exists(kKvsPrefix + ".json"));
    EXPECT_TRUE(filesystem_test::Exists(kKvsPrefix + ".hash"));
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_1.json"));
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_1.hash"));

    // A second KVS loads the flushed data.
    KvsInstanceRegistry::GetInstance().Reset();
    auto reopened{KvsBuilder{kInstanceId}
                      .Snapshot(kSnapshotId)
                      .Defaults(KvsDefaults::kRequired)
                      .KvsLoad(KvsLoad::kRequired)
                      .WorkingDir(std::string(kDataDir))
                      .Build()};
    ASSERT_TRUE(reopened);
    auto get_value_result{reopened.value()->GetValue("key1")};
    ASSERT_TRUE(get_value_result);
    ASSERT_EQ(get_value_result.value().GetType(), KvsValue::Type::kString);
    EXPECT_EQ(std::get<std::string>(get_value_result.value().GetValue()), value);

    CleanupEnvironment();
}

TEST(kvs_flush, flush_success_snapshot_rotate)
{
    PrepareEnvironment();

    // Test flush with valid data.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_1.json"));
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_1.hash"));

    // Initial flush -> snapshot ID 0.
    score::cpp::ignore = result.value()->Flush();

    // Check if SnapshotRotate was triggered on second flush --> one snapshot should be available afterwards.
    auto flush_result{result.value()->Flush()};
    ASSERT_TRUE(flush_result);
    EXPECT_TRUE(filesystem_test::Exists(kFilenamePrefix + "_1.json"));
    EXPECT_TRUE(filesystem_test::Exists(kFilenamePrefix + "_1.hash"));

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

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    std::string value{"value1"};
    ASSERT_TRUE(result.value()->SetValue("key1", KvsValue(value)));
    ASSERT_TRUE(result.value()->Flush());
    EXPECT_EQ(FilePermissions(json_path), static_cast<std::uint32_t>(kExpectedFilePermissions));
    EXPECT_EQ(FilePermissions(hash_path), static_cast<std::uint32_t>(kExpectedFilePermissions));

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
    std::string permissions_dir{kDataDir + "permissions/"};
    filesystem_test::CreateDirectories(permissions_dir);
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(permissions_dir))
                    .Build()};
    ASSERT_TRUE(result);

    filesystem_test::SetPermissions(permissions_dir, score::filesystem::Perms::kReadUser);
    auto flush_result{result.value()->Flush()};

    ASSERT_FALSE(flush_result);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_result.error()), ErrorCode::kPhysicalStorageFailure);

    CleanupEnvironment();
}

TEST(kvs_flush, flush_failure_json_writer)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // Force error in writer.ToBuffer.
    auto mock_writer{std::make_unique<score::json::IJsonWriterMock>()};
    EXPECT_CALL(*mock_writer, ToBuffer(::testing::A<const score::json::Any&>()))
        .WillOnce(
            ::testing::Return(score::Result<std::string>(score::MakeUnexpected(score::json::Error::kUnknownError))));

    (*kvs)->writer_ = std::move(mock_writer);
    auto result{kvs.value()->Flush()};
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kJsonGeneratorError);

    CleanupEnvironment();
}

TEST(kvs_snapshot_count, snapshot_count_success)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        auto count{result.value()->SnapshotCount()};
        EXPECT_TRUE(count);
        EXPECT_EQ(count.value(), i + 1);
    }
    // Test maximum capacity.
    std::ofstream(kFilenamePrefix + "_" + std::to_string(kDefaultMaxSnapshots + 1) + ".json") << "{}";
    auto count{result.value()->SnapshotCount()};
    EXPECT_TRUE(count);
    EXPECT_EQ(count.value(), kDefaultMaxSnapshots);

    CleanupEnvironment();
}

TEST(kvs_snapshot_count, snapshot_count_invalid)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // stat() fails with an error other than ENOENT, so the existence check fails.
    {
        score::os::MockGuard<::testing::NiceMock<score::os::StatMock>> stat_mock;
        EXPECT_CALL(*stat_mock, stat(::testing::_, ::testing::_, ::testing::_))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
        auto result{kvs.value()->SnapshotCount()};
        EXPECT_FALSE(result);
        EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kPhysicalStorageFailure);
    }

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_success)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    // The data received by the JSON parser should be the data listed below.
    auto keys{result.value()->GetAllKeys()};
    ASSERT_TRUE(keys);
    ASSERT_FALSE(keys.value().empty());
    auto exists_result{result.value()->KeyExists("key1")};
    ASSERT_TRUE(exists_result);
    ASSERT_FALSE(exists_result.value());
    exists_result = result.value()->KeyExists("kvs");
    ASSERT_TRUE(exists_result);
    ASSERT_TRUE(exists_result.value());

    // Create dummy data.
    const std::string json_data{R"({
        "t": "obj",
        "v": {
            "kvs_old": {
                "t": "i32",
                "v": 42
            }
        }
    })"};
    // Create hash data for json_data.
    std::ofstream out{kFilenamePrefix + "_1.json", std::ios::binary};
    out.write(json_data.data(), json_data.size());
    out.close();
    uint32_t hash{Adler32(json_data)};
    std::array<uint8_t, 4> hash_bytes{uint8_t((hash >> 24) & 0xFF),
                                      uint8_t((hash >> 16) & 0xFF),
                                      uint8_t((hash >> 8) & 0xFF),
                                      uint8_t((hash) & 0xFF)};
    std::ofstream hash_out{kFilenamePrefix + "_1.hash", std::ios::binary};
    hash_out.write(reinterpret_cast<const char*>(hash_bytes.data()), hash_bytes.size());
    hash_out.close();

    auto restore_result{result.value()->SnapshotRestore(1)};
    EXPECT_TRUE(restore_result);
    exists_result = result.value()->KeyExists("kvs_old");
    ASSERT_TRUE(exists_result);
    EXPECT_TRUE(exists_result.value());
    // Restore replaces the content, so the key loaded by Build() is gone.
    exists_result = result.value()->KeyExists("kvs");
    ASSERT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_failure_invalid_snapshot_id)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Restore snapshot ID 0 -> current KVS.
    auto restore_result{result.value()->SnapshotRestore(0)};
    ASSERT_FALSE(restore_result);
    EXPECT_EQ(static_cast<ErrorCode>(*restore_result.error()), ErrorCode::kInvalidSnapshotId);

    // Restore a snapshot ID higher than SnapshotCount (e.g. kDefaultMaxSnapshots + 1).
    restore_result = result.value()->SnapshotRestore(kDefaultMaxSnapshots + 1);
    ASSERT_FALSE(restore_result);
    EXPECT_EQ(static_cast<ErrorCode>(*restore_result.error()), ErrorCode::kInvalidSnapshotId);

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_failure_open_json)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Create empty test snapshot files.
    // Empty JSON.
    std::ofstream(kFilenamePrefix + "_1.json") << "{}";
    // Invalid hash -> trigger OpenJson() error.
    std::ofstream(kFilenamePrefix + "_1.hash") << "invalid_hash";

    auto restore_result{result.value()->SnapshotRestore(1)};
    EXPECT_FALSE(restore_result);
    // Passed by OpenJson.
    EXPECT_EQ(static_cast<ErrorCode>(*restore_result.error()), ErrorCode::kValidationFailed);

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_failure_snapshot_count)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // stat() fails with an error other than ENOENT, so the existence check fails.
    {
        score::os::MockGuard<::testing::NiceMock<score::os::StatMock>> stat_mock;
        EXPECT_CALL(*stat_mock, stat(::testing::_, ::testing::_, ::testing::_))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
        auto result{kvs.value()->SnapshotRestore(1)};
        ASSERT_FALSE(result);
        EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kPhysicalStorageFailure);
    }

    CleanupEnvironment();
}

TEST(kvs_snapshot_max_count, snapshot_max_count)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value()->SnapshotMaxCount(), kDefaultMaxSnapshots);

    CleanupEnvironment();
}

TEST(kvs_get_filename, get_kvs_filename_success)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Generate test files.
    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
    }

    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        auto filename{result.value()->KvsFilePath(SnapshotId(i))};
        EXPECT_TRUE(filename);
        EXPECT_EQ(filename.value().CStr(), kFilenamePrefix + "_" + std::to_string(i) + ".json");
    }

    CleanupEnvironment();
}

TEST(kvs_get_filename, get_kvs_filename_failure)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // Test files not available.
    auto result{kvs.value()->KvsFilePath(SnapshotId(1))};
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kFileNotFound);

    // stat() fails with an error other than ENOENT, so the existence check fails.
    {
        score::os::MockGuard<::testing::NiceMock<score::os::StatMock>> stat_mock;
        EXPECT_CALL(*stat_mock, stat(::testing::_, ::testing::_, ::testing::_))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
        result = kvs.value()->KvsFilePath(SnapshotId(1));
        ASSERT_FALSE(result);
        EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kPhysicalStorageFailure);
    }

    CleanupEnvironment();
}

TEST(kvs_get_filename, get_hashname_success)
{
    PrepareEnvironment();

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_TRUE(result);

    // Generate test files.
    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
    }

    for (size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        auto hashname{result.value()->HashFilePath(SnapshotId(i))};
        EXPECT_TRUE(hashname);
        EXPECT_EQ(hashname.value().CStr(), kFilenamePrefix + "_" + std::to_string(i) + ".hash");
    }

    CleanupEnvironment();
}

TEST(kvs_get_filename, get_hashname_failure)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // Test files not available.
    auto result{kvs.value()->HashFilePath(SnapshotId(1))};
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kFileNotFound);

    // stat() fails with an error other than ENOENT, so the existence check fails.
    {
        score::os::MockGuard<::testing::NiceMock<score::os::StatMock>> stat_mock;
        EXPECT_CALL(*stat_mock, stat(::testing::_, ::testing::_, ::testing::_))
            .WillOnce(::testing::Return(score::cpp::make_unexpected(score::os::Error::createFromErrno(EACCES))));
        result = kvs.value()->HashFilePath(SnapshotId(1));
        ASSERT_FALSE(result);
        EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kPhysicalStorageFailure);
    }

    CleanupEnvironment();
}

TEST(kvs_remove_all_keys, remove_all_keys_removes_inserted_keys)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // Insert some test key-value pairs.
    for (int i{0}; i < 5; i++)
    {
        auto set_result{kvs.value()->SetValue("key" + std::to_string(i), KvsValue(i))};
        EXPECT_TRUE(set_result);
    }

    // Remove all keys.
    auto result{kvs.value()->RemoveAllKeys()};
    EXPECT_TRUE(result);

    // Verify that all keys are removed.
    for (int i{0}; i < 5; i++)
    {
        auto get_result{kvs.value()->GetValue("key" + std::to_string(i))};
        EXPECT_FALSE(get_result);
        EXPECT_EQ(static_cast<ErrorCode>(*get_result.error()), ErrorCode::kKeyNotFound);
    }

    CleanupEnvironment();
}

TEST(kvs_remove_all_keys, remove_all_keys_empty_kvs)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value()->RemoveAllKeys());

    // Removing from an already empty KVS succeeds.
    auto result{kvs.value()->RemoveAllKeys()};
    EXPECT_TRUE(result);
    auto keys{kvs.value()->GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_TRUE(keys.value().empty());

    CleanupEnvironment();
}

TEST(kvs_remove_all_keys, remove_all_keys_keeps_defaults)
{
    RecordProperty("PartiallyVerifies", "comp_req__kvs__store_reset_api");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description", "Check removing all current keys leaves default values available separately.");
    PrepareEnvironment();
    SeedDefaults(R"({"t": "obj", "v": {"defaulted": {"t": "f64", "v": 42.0}}})");

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kRequired)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value()->GetDefaultValue("defaulted"));
    auto set_result{kvs.value()->SetValue("defaulted", KvsValue(7.0))};
    ASSERT_TRUE(set_result);

    auto result{kvs.value()->RemoveAllKeys()};
    EXPECT_TRUE(result);

    // Explicit values are gone; defaults remain accessible through GetDefaultValue().
    auto keys{kvs.value()->GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_TRUE(keys.value().empty());
    auto get_default_result{kvs.value()->GetDefaultValue("defaulted")};
    ASSERT_TRUE(get_default_result);
    EXPECT_DOUBLE_EQ(std::get<double>(get_default_result.value().GetValue()), 42.0);

    auto get_result{kvs.value()->GetValue("defaulted")};
    ASSERT_FALSE(get_result);
    EXPECT_EQ(static_cast<ErrorCode>(*get_result.error()), ErrorCode::kKeyNotFound);

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_reverts_to_state_at_open)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // "kvs" is loaded from the test file by PrepareEnvironment().
    auto exists_result{kvs.value()->KeyExists("kvs")};
    ASSERT_TRUE(exists_result);
    ASSERT_TRUE(exists_result.value());

    // Make pending changes of every kind: add, modify, remove.
    ASSERT_TRUE(kvs.value()->SetValue("added", KvsValue(1.0)));
    ASSERT_TRUE(kvs.value()->SetValue("kvs", KvsValue(99.0)));
    ASSERT_TRUE(kvs.value()->RemoveAllKeys());

    auto result{kvs.value()->DiscardPendingChanges()};
    EXPECT_TRUE(result);

    // Back to what Build() loaded.
    exists_result = kvs.value()->KeyExists("added");
    ASSERT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());
    exists_result = kvs.value()->KeyExists("kvs");
    ASSERT_TRUE(exists_result);
    ASSERT_TRUE(exists_result.value());
    auto get_result{kvs.value()->GetValue("kvs")};
    ASSERT_TRUE(get_result);
    EXPECT_EQ(get_result.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(get_result.value().GetValue()), 2);

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_reverts_to_last_flush)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value()->RemoveAllKeys());
    ASSERT_TRUE(kvs.value()->SetValue("kept", KvsValue(1.0)));
    ASSERT_TRUE(kvs.value()->Flush());

    // Changes made after the flush are the pending ones.
    ASSERT_TRUE(kvs.value()->SetValue("kept", KvsValue(2.0)));
    ASSERT_TRUE(kvs.value()->SetValue("pending", KvsValue(3.0)));

    auto result{kvs.value()->DiscardPendingChanges()};
    EXPECT_TRUE(result);

    auto exists_result{kvs.value()->KeyExists("pending")};
    ASSERT_TRUE(exists_result);
    EXPECT_FALSE(exists_result.value());
    auto get_result{kvs.value()->GetValue("kept")};
    ASSERT_TRUE(get_result);
    EXPECT_DOUBLE_EQ(std::get<double>(get_result.value().GetValue()), 1.0);

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_without_changes_is_noop)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    const auto keys_before{kvs.value()->GetAllKeys()};
    ASSERT_TRUE(keys_before);

    EXPECT_TRUE(kvs.value()->DiscardPendingChanges());
    EXPECT_EQ(kvs.value()->GetAllKeys().value().size(), keys_before.value().size());

    // Repeated calls stay stable.
    EXPECT_TRUE(kvs.value()->DiscardPendingChanges());
    EXPECT_EQ(kvs.value()->GetAllKeys().value().size(), keys_before.value().size());

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_without_persisted_file_yields_empty)
{
    PrepareEnvironment();

    // No KVS file: opened as Optional and never flushed.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value()->SetValue("pending", KvsValue(1.0)));

    EXPECT_TRUE(kvs.value()->DiscardPendingChanges());
    auto keys{kvs.value()->GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_TRUE(keys.value().empty());

    CleanupEnvironment();
}

TEST(kvs_discard_pending_changes, discard_reports_corrupted_storage)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // Corrupt the hash of the persisted file after opening.
    std::fstream corrupt_hash_file{kKvsPrefix + ".hash", std::ios::in | std::ios::out | std::ios::binary};
    corrupt_hash_file.seekp(0);
    corrupt_hash_file.put(0xFF);
    corrupt_hash_file.close();

    auto result{kvs.value()->DiscardPendingChanges()};
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kValidationFailed);

    CleanupEnvironment();
}

TEST(kvs_get_storage_file_size, size_matches_files_on_disk)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kRequired)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    const auto expected{filesystem_test::FileSize(kKvsPrefix + ".json") +
                        filesystem_test::FileSize(kKvsPrefix + ".hash")};

    auto result{kvs.value()->GetStorageFileSize()};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), expected);

    CleanupEnvironment();
}

TEST(kvs_get_storage_file_size, size_without_files_is_zero)
{
    PrepareEnvironment();

    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    auto result{kvs.value()->GetStorageFileSize()};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value(), 0U);

    CleanupEnvironment();
}

TEST(kvs_get_storage_file_size, size_reflects_storage_not_pending_changes)
{
    PrepareEnvironment();

    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    system(("rm -rf " + kKvsPrefix + ".hash").c_str());

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // Unflushed changes are invisible to the size query.
    ASSERT_TRUE(kvs.value()->SetValue("pending", KvsValue(1.0)));
    auto before{kvs.value()->GetStorageFileSize()};
    ASSERT_TRUE(before);
    EXPECT_EQ(before.value(), 0U);

    ASSERT_TRUE(kvs.value()->Flush());

    auto after{kvs.value()->GetStorageFileSize()};
    ASSERT_TRUE(after);
    EXPECT_GT(after.value(), 0U);

    CleanupEnvironment();
}

TEST(kvs_get_storage_file_size, size_failure_unreadable_path)
{
    PrepareEnvironment();

    auto kvs{KvsBuilder{kInstanceId}
                 .Snapshot(kSnapshotId)
                 .Defaults(KvsDefaults::kOptional)
                 .KvsLoad(KvsLoad::kOptional)
                 .WorkingDir(std::string(kDataDir))
                 .Build()};
    ASSERT_TRUE(kvs);

    // A directory in place of the data file cannot be sized as a regular file.
    system(("rm -rf " + kKvsPrefix + ".json").c_str());
    filesystem_test::CreateDirectory(kKvsPrefix + ".json");

    auto result{kvs.value()->GetStorageFileSize()};
    EXPECT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kPhysicalStorageFailure);

    CleanupEnvironment();
}

TEST(kvs_max_storage_bytes, flush_succeeds_without_configured_limit)
{
    // The builder default is an unset optional, meaning no limit is enforced.
    // Data far larger than kTestMaxStorageBytes must therefore still flush.
    const std::string test_dir{"./kvs_no_limit_test/"};
    CleanupEnvironment(test_dir);

    KvsBuilder builder{kInstanceId};
    builder.WorkingDir(std::string(test_dir));
    auto open_result{builder.Build()};
    ASSERT_TRUE(open_result);
    Kvs& kvs{*open_result.value()};

    const std::string large_data(kTestMaxStorageBytes * 4U, 'a');
    auto set_result{kvs.SetValue("large_data", KvsValue(large_data.c_str()))};
    ASSERT_TRUE(set_result);

    auto flush_result{kvs.Flush()};
    EXPECT_TRUE(flush_result);

    CleanupEnvironment(test_dir);
}

TEST(kvs_max_storage_bytes, flush_fails_when_storage_limit_exceeded)
{
    const std::string test_dir{"./kvs_storage_test/"};
    CleanupEnvironment(test_dir);

    KvsBuilder builder{kInstanceId};
    builder.WorkingDir(std::string(test_dir));
    builder.MaxStorageBytes(kTestMaxStorageBytes);
    auto open_result{builder.Build()};
    ASSERT_TRUE(open_result);
    Kvs& kvs{*open_result.value()};

    // Add data close to the limit.
    // The JSON structure (key, type info, braces) and the hash file add overhead.
    // Keep the payload below the maximum.
    const size_t overhead_estimate{150U};
    const std::string large_data(kTestMaxStorageBytes - overhead_estimate, 'a');

    auto set_result1{kvs.SetValue("large_data", KvsValue(large_data.c_str()))};
    ASSERT_TRUE(set_result1);

    // The first flush still fits.
    auto flush_result1{kvs.Flush()};
    ASSERT_TRUE(flush_result1);

    // A little more data pushes the total over the limit.
    auto set_result2{kvs.SetValue("extra_data", KvsValue("this should not fit"))};
    ASSERT_TRUE(set_result2);

    auto flush_result2{kvs.Flush()};
    ASSERT_FALSE(flush_result2);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_result2.error()), ErrorCode::kOutOfStorageSpace);

    CleanupEnvironment(test_dir);
}

TEST(kvs_check_size, check_size_within_limit_succeeds)
{
    const std::string test_dir{"./kvs_check_size_within_test/"};
    CleanupEnvironment(test_dir);

    KvsBuilder builder{InstanceId(1)};
    builder.WorkingDir(std::string(test_dir));
    builder.MaxStorageBytes(kTestMaxStorageBytes);
    auto open_result{builder.Build()};
    ASSERT_TRUE(open_result);
    Kvs& kvs{*open_result.value()};

    auto set_result{kvs.SetValue("key", KvsValue("some_data"))};
    ASSERT_TRUE(set_result);

    auto check_result{kvs.CalculatePotentialSize()};
    ASSERT_TRUE(check_result) << "CalculatePotentialSize should succeed for data within limits";
    EXPECT_GT(check_result.value(), 0U);
    EXPECT_LT(check_result.value(), kTestMaxStorageBytes);

    CleanupEnvironment(test_dir);
}

TEST(kvs_check_size, check_size_exceeding_limit_fails)
{
    const std::string test_dir{"./kvs_check_size_exceeding_test/"};
    CleanupEnvironment(test_dir);

    KvsBuilder builder{InstanceId(2)};
    builder.WorkingDir(std::string(test_dir));
    builder.MaxStorageBytes(kTestMaxStorageBytes);
    auto open_result{builder.Build()};
    ASSERT_TRUE(open_result);
    Kvs& kvs{*open_result.value()};

    const std::string large_data(kTestMaxStorageBytes, 'x');
    auto set_result{kvs.SetValue("oversized_key", KvsValue(large_data.c_str()))};
    ASSERT_TRUE(set_result);

    auto check_result{kvs.CalculatePotentialSize()};
    ASSERT_FALSE(check_result) << "CalculatePotentialSize should fail when the storage limit is exceeded";
    EXPECT_EQ(static_cast<ErrorCode>(*check_result.error()), ErrorCode::kOutOfStorageSpace);

    CleanupEnvironment(test_dir);
}

TEST(kvs_max_storage_bytes, flush_without_limit_does_not_measure_storage)
{
    // Without a configured limit, Flush() must not measure the persisted files at all.
    // A KVS that does not use the limit then behaves exactly as before this feature existed.
    // A directory placed where a defaults file is expected makes any size measurement fail.
    // A successful flush here proves that no measurement happened.
    const std::string test_dir{"./kvs_no_limit_no_measure_test/"};
    CleanupEnvironment(test_dir);

    KvsBuilder builder{kInstanceId};
    builder.WorkingDir(std::string(test_dir));
    auto open_result{builder.Build()};
    ASSERT_TRUE(open_result);
    Kvs& kvs{*open_result.value()};

    // Obstruct the measurement only after building, so Build() itself is unaffected
    filesystem_test::CreateDirectories(test_dir + "kvs_" + std::to_string(kInstance) + "_default.hash");

    auto set_result{kvs.SetValue("key", KvsValue("value"))};
    ASSERT_TRUE(set_result);

    auto flush_result{kvs.Flush()};
    EXPECT_TRUE(flush_result);

    CleanupEnvironment(test_dir);
}

TEST(kvs_max_storage_bytes, flush_with_limit_measures_storage)
{
    // Counterpart to flush_without_limit_does_not_measure_storage.
    // With a limit set, the same obstruction makes Flush() fail, so the obstruction really does break the measurement.
    // Without this test, the one above could pass for the wrong reason.
    // The limit is far above the data size, so the limit itself is not reached.
    const std::string test_dir{"./kvs_limit_measures_test/"};
    CleanupEnvironment(test_dir);

    KvsBuilder builder{kInstanceId};
    builder.WorkingDir(std::string(test_dir));
    builder.MaxStorageBytes(kTestMaxStorageBytes * 1000U);
    auto open_result{builder.Build()};
    ASSERT_TRUE(open_result);
    Kvs& kvs{*open_result.value()};

    filesystem_test::CreateDirectories(test_dir + "kvs_" + std::to_string(kInstance) + "_default.hash");

    auto set_result{kvs.SetValue("key", KvsValue("value"))};
    ASSERT_TRUE(set_result);

    auto flush_result{kvs.Flush()};
    ASSERT_FALSE(flush_result);
    EXPECT_EQ(static_cast<ErrorCode>(*flush_result.error()), ErrorCode::kPhysicalStorageFailure);

    CleanupEnvironment(test_dir);
}

TEST(kvs_check_size, check_size_without_limit_reports_size)
{
    // CalculatePotentialSize() shares SerializeAndCheck() with Flush().
    // Skipping the measurement there when no limit is set must not stop the dry run from reporting a size.
    // Without a limit, it measures on its own and returns a real, non-zero size.
    const std::string test_dir{"./kvs_check_size_no_limit_test/"};
    CleanupEnvironment(test_dir);

    KvsBuilder builder{InstanceId(3)};
    builder.WorkingDir(std::string(test_dir));
    auto open_result{builder.Build()};
    ASSERT_TRUE(open_result);
    Kvs& kvs{*open_result.value()};

    auto set_result{kvs.SetValue("key", KvsValue("some_data"))};
    ASSERT_TRUE(set_result);

    auto check_result{kvs.CalculatePotentialSize()};
    ASSERT_TRUE(check_result) << "CalculatePotentialSize should succeed when no limit is configured";
    EXPECT_GT(check_result.value(), 0U);

    CleanupEnvironment(test_dir);
}

// comp_req__kvs__snapshot_max_num_cfg:
// "The component shall maintain a configurable maximum number of snapshots.
// The maximum number shall be in the range <0..3>."
// The tests below cover the boundary values the component integration tests exercise (0, 1, 3).
// They also cover values above the range, which are clamped.

TEST(kvs_snapshot_max_count, snapshot_max_count_configurable)
{
    // The reported maximum must be the configured one, not the compile-time default.
    for (const std::size_t configured_max : {std::size_t{0U}, std::size_t{1U}, kMaxSnapshotsUpperBound})
    {
        PrepareEnvironment();
        auto result{KvsBuilder{kInstanceId}
                        .Snapshot(kSnapshotId)
                        .Defaults(KvsDefaults::kOptional)
                        .KvsLoad(KvsLoad::kOptional)
                        .WorkingDir(std::string(kDataDir))
                        .SnapshotMaxCount(configured_max)
                        .Build()};
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value()->SnapshotMaxCount(), configured_max);
        CleanupEnvironment();
    }
}

TEST(kvs_snapshot_rotate, snapshot_rotate_respects_configured_max)
{
    PrepareEnvironment();

    // Rotation must stop at the configured maximum.
    // A configuration that is only reported but not enforced would leave this snapshot behind.
    const std::size_t configured_max = 1U;
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(configured_max)
                    .Build()};
    ASSERT_TRUE(result);

    // Create test snapshot files up to the configured maximum.
    // Tag them, so that the rotation can be followed by content and not only by existence.
    for (std::size_t i{0U}; i <= configured_max; i++)
    {
        PlantGeneration(i);
    }

    ASSERT_TRUE(result.value()->Flush());

    // Rotation happened: the previous snapshot 0 moved into the first snapshot slot.
    // Flush() then wrote a new snapshot 0.
    // Without this the test would also pass if nothing rotated.
    EXPECT_EQ(GenerationJson(1), GenerationTag(0));
    EXPECT_TRUE(FileExists(score::filesystem::Path{kFilenamePrefix + "_0.json"}));
    EXPECT_NE(GenerationJson(0), GenerationTag(0));

    // No snapshot beyond the configured maximum may be created.
    // If rotation used the compile-time default instead of the configured value, _1 would have been rotated into _2.
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_" + std::to_string(configured_max + 1U) + ".json"));
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_" + std::to_string(configured_max + 1U) + ".hash"));

    CleanupEnvironment();
}

TEST(kvs_snapshot_count, snapshot_count_caps_at_configured_max)
{
    PrepareEnvironment();

    // Counting must use the configured maximum as upper bound.
    // Otherwise, the count could report snapshots that rotation is not allowed to maintain.
    const std::size_t configured_max = 1U;
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(configured_max)
                    .Build()};
    ASSERT_TRUE(result);

    for (std::size_t i{0U}; i < kDefaultMaxSnapshots; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
    }

    auto count{result.value()->SnapshotCount()};
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

    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(0U)
                    .Build()};
    ASSERT_TRUE(result);

    std::string value{"value1"};
    ASSERT_TRUE(result.value()->SetValue("key1", KvsValue(value)));

    // The current KVS is persisted ...
    auto flush_result{result.value()->Flush()};
    EXPECT_TRUE(flush_result);
    EXPECT_TRUE(filesystem_test::Exists(kKvsPrefix + ".json"));
    EXPECT_TRUE(filesystem_test::Exists(kKvsPrefix + ".hash"));

    // ... but no previous generation is ever created ...
    auto second_flush_result{result.value()->Flush()};
    EXPECT_TRUE(second_flush_result);
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_1.json"));
    EXPECT_FALSE(filesystem_test::Exists(kFilenamePrefix + "_1.hash"));

    // ... and no snapshot is reported.
    auto count{result.value()->SnapshotCount()};
    ASSERT_TRUE(count);
    EXPECT_EQ(count.value(), 0U);

    CleanupEnvironment();
}

TEST(kvs_snapshot_restore, snapshot_restore_beyond_configured_max)
{
    PrepareEnvironment();

    // Restoring a snapshot outside the configured window must be rejected, even if an older file is still present.
    const std::size_t configured_max = 1U;
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(configured_max)
                    .Build()};
    ASSERT_TRUE(result);

    for (std::size_t i{0U}; i <= configured_max + 1U; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
    }

    auto restore_result{result.value()->SnapshotRestore(SnapshotId(configured_max + 1U))};
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
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(configured_max)
                    .Build()};
    ASSERT_TRUE(result);

    auto result_default{KvsBuilder{InstanceId(kInstanceId.id + 1U)}
                            .Snapshot(kSnapshotId)
                            .Defaults(KvsDefaults::kOptional)
                            .KvsLoad(KvsLoad::kOptional)
                            .WorkingDir(std::string(kDataDir))
                            .Build()};
    ASSERT_TRUE(result_default);
    KvsInstanceRegistry::GetInstance().Reset();
    Kvs kvs_moved{std::move(*result.value())};
    EXPECT_EQ(kvs_moved.SnapshotMaxCount(), configured_max);
    Kvs kvs_assigned{std::move(*result_default.value())};
    ASSERT_EQ(kvs_assigned.SnapshotMaxCount(), kDefaultMaxSnapshots);

    kvs_assigned = std::move(kvs_moved);
    EXPECT_EQ(kvs_assigned.SnapshotMaxCount(), configured_max);

    CleanupEnvironment();
}

TEST(kvs_snapshot_max_count, snapshot_max_count_clamped_to_upper_bound)
{
    // comp_req__kvs__snapshot_max_num_cfg: a request above the range is clamped to kMaxSnapshotsUpperBound.
    // The first value outside the range and a far larger one must both be clamped.
    // This shows the bound is enforced for every value, not just one.
    for (const std::size_t requested : {kMaxSnapshotsUpperBound + 1U, std::size_t{100U}})
    {
        PrepareEnvironment();
        auto result{KvsBuilder{kInstanceId}
                        .Snapshot(kSnapshotId)
                        .Defaults(KvsDefaults::kOptional)
                        .KvsLoad(KvsLoad::kOptional)
                        .WorkingDir(std::string(kDataDir))
                        .SnapshotMaxCount(requested)
                        .Build()};

        ASSERT_TRUE(result);
        EXPECT_EQ(result.value()->SnapshotMaxCount(), kMaxSnapshotsUpperBound);
        CleanupEnvironment();
    }

    PrepareEnvironment();

    // Rotation follows the clamped value: no snapshot beyond the upper bound is created.
    auto result{KvsBuilder{kInstanceId}
                    .Snapshot(kSnapshotId)
                    .Defaults(KvsDefaults::kOptional)
                    .KvsLoad(KvsLoad::kOptional)
                    .WorkingDir(std::string(kDataDir))
                    .SnapshotMaxCount(100U)
                    .Build()};
    ASSERT_TRUE(result);
    for (std::size_t i{0U}; i <= kMaxSnapshotsUpperBound; i++)
    {
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".json") << "{}";
        std::ofstream(kFilenamePrefix + "_" + std::to_string(i) + ".hash") << "{}";
    }
    ASSERT_TRUE(result.value()->Flush());
    EXPECT_FALSE(
        filesystem_test::Exists(kFilenamePrefix + "_" + std::to_string(kMaxSnapshotsUpperBound + 1U) + ".json"));

    CleanupEnvironment();
}

TEST(kvs_max_storage_bytes, storage_size_counts_only_configured_snapshots)
{
    // The storage estimate must count only the snapshot files this instance keeps.
    // With one snapshot configured, rotation is about to overwrite an existing _1 file.
    // So it must not count against the limit.
    // Counting the default number of snapshots instead of the configured one would reject this flush although it fits.
    const std::string test_dir{"./kvs_storage_configured_snapshots_test/"};
    CleanupEnvironment(test_dir);

    KvsBuilder builder{kInstanceId};
    builder.WorkingDir(std::string(test_dir));
    builder.SnapshotMaxCount(1U);
    builder.MaxStorageBytes(kTestMaxStorageBytes);
    auto open_result{builder.Build()};
    ASSERT_TRUE(open_result);
    Kvs& kvs{*open_result.value()};

    // A large file in snapshot slot 1, which the next rotation overwrites.
    filesystem_test::CreateDirectories(test_dir);
    std::ofstream(test_dir + "kvs_" + std::to_string(kInstance) + "_1.json") << std::string(kTestMaxStorageBytes, 'x');

    auto set_result{kvs.SetValue("key", KvsValue("value"))};
    ASSERT_TRUE(set_result);

    auto flush_result{kvs.Flush()};
    EXPECT_TRUE(flush_result);

    CleanupEnvironment(test_dir);
}

namespace
{
// Two handles to the same instance, with a pending, unflushed change.
std::pair<std::shared_ptr<Kvs>, std::shared_ptr<Kvs>> BuildSharedHandlesWithPendingChange()
{
    const auto build = []() {
        auto result{KvsBuilder{kInstanceId}.WorkingDir(std::string(kDataDir)).Build()};
        EXPECT_TRUE(result);
        return std::move(result.value());
    };
    auto observer{build()};
    auto actor{build()};
    EXPECT_TRUE(observer->SetValue("pending", KvsValue(int32_t{1})));
    return {std::move(observer), std::move(actor)};
}
}  // namespace

TEST(kvs_shared_state, reset_is_visible_through_other_handle)
{
    PrepareEnvironment();
    auto [observer_handle, actor_handle]{BuildSharedHandlesWithPendingChange()};
    auto& observer{*observer_handle};
    auto& actor{*actor_handle};

    ASSERT_TRUE(actor.Reset());

    auto keys{observer.GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_TRUE(keys.value().empty());

    CleanupEnvironment();
}

TEST(kvs_shared_state, remove_all_keys_is_visible_through_other_handle)
{
    PrepareEnvironment();
    auto [observer_handle, actor_handle]{BuildSharedHandlesWithPendingChange()};
    auto& observer{*observer_handle};
    auto& actor{*actor_handle};

    ASSERT_TRUE(actor.RemoveAllKeys());

    auto keys{observer.GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_TRUE(keys.value().empty());

    CleanupEnvironment();
}

TEST(kvs_shared_state, discard_pending_changes_is_visible_through_other_handle)
{
    PrepareEnvironment();
    auto [observer_handle, actor_handle]{BuildSharedHandlesWithPendingChange()};
    auto& observer{*observer_handle};
    auto& actor{*actor_handle};

    // The change pending through the observer is dropped, the persisted "kvs" key is reloaded.
    ASSERT_TRUE(actor.DiscardPendingChanges());

    auto pending_exists{observer.KeyExists("pending")};
    ASSERT_TRUE(pending_exists);
    EXPECT_FALSE(pending_exists.value());
    auto kvs_exists{observer.KeyExists("kvs")};
    ASSERT_TRUE(kvs_exists);
    EXPECT_TRUE(kvs_exists.value());

    CleanupEnvironment();
}

TEST(kvs_shared_state, snapshot_restore_is_visible_through_other_handle)
{
    PrepareEnvironment(kDataDir,
                       "kvs_",
                       kInstance,
                       kDefaultJson,
                       kKvsJson,
                       R"({"t": "obj", "v": {"kvs_old": {"t": "i32", "v": 42}}})");
    auto [observer_handle, actor_handle]{BuildSharedHandlesWithPendingChange()};
    auto& observer{*observer_handle};
    auto& actor{*actor_handle};

    ASSERT_TRUE(actor.SnapshotRestore(SnapshotId{1ul}));

    auto restored{observer.GetValue("kvs_old")};
    ASSERT_TRUE(restored);
    ASSERT_EQ(restored.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(restored.value().GetValue()), 42);
    auto pending_exists{observer.KeyExists("pending")};
    ASSERT_TRUE(pending_exists);
    EXPECT_FALSE(pending_exists.value());

    CleanupEnvironment();
}

TEST(kvs_kvsvalue, const_char_ptr_nullptr_precondition)
{
    // A null C string violates the precondition and must abort through the S-CORE assertion handler.
    const char* null_str{nullptr};
    EXPECT_DEATH(KvsValue{null_str}, "");
}

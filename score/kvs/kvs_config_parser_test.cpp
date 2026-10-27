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

/* Unit tests for the configuration-by-file parser: STORAGE_CONFIG_PATH discovery and the binary-relative
 * fallback, document parsing and validation (including every invalid row of the error table), and
 * instance lookup via KvsConfigParser::load_entry().
 * Design reference: others/config_by_file.md, section "As implemented (issue #411)". */

#include "kvs_config_general_test.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <string>

using namespace score::mw::per::kvs;
using score::mw::per::kvs::test::Doc;
using score::mw::per::kvs::test::ExpectErrorCode;
using score::mw::per::kvs::test::kTestInstanceId;
using score::mw::per::kvs::test::KvsConfigFixture;

namespace
{

/* Returns the path discovery must resolve to when STORAGE_CONFIG_PATH is unset: the binary-relative
 * fallback if that file exists, otherwise nothing. Lets tests stay correct on any machine. */
std::optional<std::string> ExpectedFallbackPath()
{
    const std::optional<std::string> bin_dir = KvsConfigParser().running_binary_directory();
    if (!bin_dir.has_value())
    {
        return std::nullopt;
    }
    const std::filesystem::path candidate =
        std::filesystem::path(bin_dir.value()) / ".." / "etc" / std::string(kKvsConfigFileName);
    std::error_code ec;
    if (std::filesystem::exists(candidate, ec) && !ec)
    {
        return candidate.string();
    }
    return std::nullopt;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
/* Discovery: STORAGE_CONFIG_PATH resolution and the binary-relative fallback */
////////////////////////////////////////////////////////////////////////////////

using KvsConfigDiscovery = KvsConfigFixture;

TEST_F(KvsConfigDiscovery, UnsetEnvironmentWithoutFallbackFileIsNotAnError)
{
    auto res = KvsConfigParser().discover_kvs_config_file_path();
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), ExpectedFallbackPath());
}

TEST_F(KvsConfigDiscovery, EmptyEnvironmentValueIsTreatedAsUnset)
{
    SetConfigEnv("");
    auto res = KvsConfigParser().discover_kvs_config_file_path();
    ASSERT_TRUE(res);
    EXPECT_EQ(res.value(), ExpectedFallbackPath());
}

TEST_F(KvsConfigDiscovery, DirectoryContainingConfigFileResolvesToFile)
{
    const std::string dir = SubDir("cfg_dir");
    WriteFile("cfg_dir/" + std::string(kKvsConfigFileName), "{}");
    SetConfigEnv(dir);

    auto res = KvsConfigParser().discover_kvs_config_file_path();
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    EXPECT_EQ(res.value().value(), (std::filesystem::path(dir) / std::string(kKvsConfigFileName)).string());
}

TEST_F(KvsConfigDiscovery, DirectoryWithTrailingSlashResolvesToFile)
{
    const std::string dir = SubDir("cfg_dir_slash");
    WriteFile("cfg_dir_slash/" + std::string(kKvsConfigFileName), "{}");
    SetConfigEnv(dir + "/");

    auto res = KvsConfigParser().discover_kvs_config_file_path();
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    EXPECT_EQ(std::filesystem::path(res.value().value()).filename().string(), std::string(kKvsConfigFileName));
}

TEST_F(KvsConfigDiscovery, DirectoryWithoutConfigFileIsFileNotFound)
{
    const std::string dir = SubDir("empty_cfg_dir");
    SetConfigEnv(dir);

    auto res = KvsConfigParser().discover_kvs_config_file_path();
    ExpectErrorCode(res, ErrorCode::FileNotFound);
}

TEST_F(KvsConfigDiscovery, FileTargetResolvesToItself)
{
    const std::string file = WriteFile("custom/any_name.json", "{}");
    SetConfigEnv(file);

    auto res = KvsConfigParser().discover_kvs_config_file_path();
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    EXPECT_EQ(res.value().value(), file);
}

TEST_F(KvsConfigDiscovery, MissingFileTargetIsFileNotFound)
{
    SetConfigEnv((std::filesystem::path(base_dir_) / "does_not_exist.json").string());

    auto res = KvsConfigParser().discover_kvs_config_file_path();
    ExpectErrorCode(res, ErrorCode::FileNotFound);
}

TEST_F(KvsConfigDiscovery, MissingDirectoryTargetIsFileNotFound)
{
    SetConfigEnv((std::filesystem::path(base_dir_) / "no_such_dir").string());

    auto res = KvsConfigParser().discover_kvs_config_file_path();
    ExpectErrorCode(res, ErrorCode::FileNotFound);
}

TEST_F(KvsConfigDiscovery, RunningBinaryDirectoryIsAvailableOnLinux)
{
    const auto bin_dir = KvsConfigParser().running_binary_directory();
    ASSERT_TRUE(bin_dir.has_value());
    EXPECT_FALSE(bin_dir.value().empty());
}

////////////////////////////////////////////////////////////////////////////////
/* Parsing: valid documents */
////////////////////////////////////////////////////////////////////////////////

using KvsConfigParse = KvsConfigFixture;

TEST_F(KvsConfigParse, FullEntryAndPartialEntryParseAllFields)
{
    const std::string path = WriteConfig(Doc(R"([
        {"instance_id": 2, "storage_path": "./data", "need_defaults": "required", "need_kvs": "optional",
         "snapshot_max_count": 0, "max_storage_size_bytes": 4096},
        {"instance_id": 1, "storage_path": "./data_2"}
    ])"));

    auto res = KvsConfigParser().parse_kvs_config_file(path);
    ASSERT_TRUE(res);
    const auto& entries = res.value();
    ASSERT_EQ(entries.size(), 2u);

    EXPECT_EQ(entries[0].instance_id, 2u);
    EXPECT_EQ(entries[0].storage_path, std::optional<std::string>{"./data"});
    EXPECT_EQ(entries[0].need_defaults, std::optional<KvsConfigNeedMode>{KvsConfigNeedMode::Required});
    EXPECT_EQ(entries[0].need_kvs, std::optional<KvsConfigNeedMode>{KvsConfigNeedMode::Optional});
    EXPECT_EQ(entries[0].snapshot_max_count, std::optional<std::size_t>{0u});
    EXPECT_EQ(entries[0].max_storage_size_bytes, std::optional<std::size_t>{4096u});

    EXPECT_EQ(entries[1].instance_id, 1u);
    EXPECT_EQ(entries[1].storage_path, std::optional<std::string>{"./data_2"});
    EXPECT_FALSE(entries[1].need_defaults.has_value());
    EXPECT_FALSE(entries[1].need_kvs.has_value());
    EXPECT_FALSE(entries[1].snapshot_max_count.has_value());
    EXPECT_FALSE(entries[1].max_storage_size_bytes.has_value());
}

TEST_F(KvsConfigParse, MinimalEntryLeavesOptionalFieldsUnset)
{
    const std::string path = WriteConfig(Doc(R"([{"instance_id": 0}])"));

    auto res = KvsConfigParser().parse_kvs_config_file(path);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value().size(), 1u);
    const KvsConfigEntry& entry = res.value()[0];
    EXPECT_EQ(entry.instance_id, 0u);
    EXPECT_FALSE(entry.storage_path.has_value());
    EXPECT_FALSE(entry.need_defaults.has_value());
    EXPECT_FALSE(entry.need_kvs.has_value());
    EXPECT_FALSE(entry.snapshot_max_count.has_value());
    EXPECT_FALSE(entry.read_only.has_value());
    EXPECT_FALSE(entry.max_storage_size_bytes.has_value());
}

TEST_F(KvsConfigParse, EmptyListIsValid)
{
    const std::string path = WriteConfig(Doc("[]"));

    auto res = KvsConfigParser().parse_kvs_config_file(path);
    ASSERT_TRUE(res);
    EXPECT_TRUE(res.value().empty());
}

TEST_F(KvsConfigParse, BoundaryValuesAreAccepted)
{
    /* snapshot_max_count 0 and 3 and instance_id 0 are all in range. */
    const std::string path = WriteConfig(Doc(R"([
        {"instance_id": 0, "snapshot_max_count": 3},
        {"instance_id": 1, "snapshot_max_count": 0}
    ])"));

    auto res = KvsConfigParser().parse_kvs_config_file(path);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value().size(), 2u);
    EXPECT_EQ(res.value()[0].snapshot_max_count, std::optional<std::size_t>{3u});
    EXPECT_EQ(res.value()[1].snapshot_max_count, std::optional<std::size_t>{0u});
}

TEST_F(KvsConfigParse, OptionalEnumValuesMapToExpectedModes)
{
    const std::string path = WriteConfig(Doc(R"([
        {"instance_id": 4, "need_defaults": "optional", "need_kvs": "required"}
    ])"));

    auto res = KvsConfigParser().parse_kvs_config_file(path);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value().size(), 1u);
    EXPECT_EQ(res.value()[0].need_defaults, std::optional<KvsConfigNeedMode>{KvsConfigNeedMode::Optional});
    EXPECT_EQ(res.value()[0].need_kvs, std::optional<KvsConfigNeedMode>{KvsConfigNeedMode::Required});
}

TEST_F(KvsConfigParse, AccessModeMapsToReadOnlyFlag)
{
    const std::string path = WriteConfig(Doc(R"([
        {"instance_id": 5, "access_mode": "read_only"},
        {"instance_id": 6, "access_mode": "read_write"}
    ])"));

    auto res = KvsConfigParser().parse_kvs_config_file(path);
    ASSERT_TRUE(res);
    ASSERT_EQ(res.value().size(), 2u);
    EXPECT_EQ(res.value()[0].read_only, std::optional<bool>{true});
    EXPECT_EQ(res.value()[1].read_only, std::optional<bool>{false});
}

TEST_F(KvsConfigParse, MissingFileIsKvsFileReadError)
{
    auto res = KvsConfigParser().parse_kvs_config_file((std::filesystem::path(base_dir_) / "absent.json").string());
    ExpectErrorCode(res, ErrorCode::KvsFileReadError);
}

////////////////////////////////////////////////////////////////////////////////
/* Parsing: every invalid row of the error table in the design document */
////////////////////////////////////////////////////////////////////////////////

struct BadDocumentCase
{
    std::string name;
    std::string json;
    ErrorCode expected;
};

class KvsConfigParseInvalid : public KvsConfigFixture, public ::testing::WithParamInterface<BadDocumentCase>
{
};

TEST_P(KvsConfigParseInvalid, RejectsDocumentWithExpectedErrorCode)
{
    const BadDocumentCase& param = GetParam();
    const std::string path = WriteConfig(param.json);

    auto res = KvsConfigParser().parse_kvs_config_file(path);
    ExpectErrorCode(res, param.expected);
}

INSTANTIATE_TEST_SUITE_P(
    ErrorTable,
    KvsConfigParseInvalid,
    ::testing::Values(
        BadDocumentCase{"BadJsonSyntax", "{", ErrorCode::JsonParserError},
        BadDocumentCase{"RootIsArray", "[]", ErrorCode::InvalidConfiguration},
        BadDocumentCase{"FormatVersionMissing", R"({"key_value_storage_list": []})", ErrorCode::InvalidConfiguration},
        BadDocumentCase{"FormatVersionTwo",
                        R"({"format_version": 2, "key_value_storage_list": []})",
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"FormatVersionZero",
                        R"({"format_version": 0, "key_value_storage_list": []})",
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"FormatVersionString",
                        R"({"format_version": "1", "key_value_storage_list": []})",
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"ListMissing", R"({"format_version": 1})", ErrorCode::InvalidConfiguration},
        BadDocumentCase{"ListIsObject",
                        R"({"format_version": 1, "key_value_storage_list": {}})",
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"UnknownRootKey",
                        R"({"format_version": 1, "key_value_storage_list": [], "extra": true})",
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"EntryIsNotObject", Doc("[5]"), ErrorCode::InvalidConfiguration},
        BadDocumentCase{"EntryMissingInstanceId", Doc(R"([{"storage_path": "./a"}])"), ErrorCode::InvalidConfiguration},
        BadDocumentCase{"InstanceIdNegative", Doc(R"([{"instance_id": -1}])"), ErrorCode::InvalidConfiguration},
        BadDocumentCase{"InstanceIdString", Doc(R"([{"instance_id": "1"}])"), ErrorCode::InvalidConfiguration},
        BadDocumentCase{"UnknownEntryKeyTypo",
                        Doc(R"([{"instance_id": 1, "snapshot_max_cnt": 2}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"StoragePathNotString",
                        Doc(R"([{"instance_id": 1, "storage_path": 7}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"NeedDefaultsUnknownValue",
                        Doc(R"([{"instance_id": 1, "need_defaults": "without"}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"NeedDefaultsNotString",
                        Doc(R"([{"instance_id": 1, "need_defaults": true}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"NeedKvsUnknownValue",
                        Doc(R"([{"instance_id": 1, "need_kvs": "maybe"}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"AccessModeUnknownValue",
                        Doc(R"([{"instance_id": 1, "access_mode": "write_only"}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"AccessModeNotString",
                        Doc(R"([{"instance_id": 1, "access_mode": 1}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"SnapshotMaxCountAboveRange",
                        Doc(R"([{"instance_id": 1, "snapshot_max_count": 4}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"SnapshotMaxCountNegative",
                        Doc(R"([{"instance_id": 1, "snapshot_max_count": -1}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"SnapshotMaxCountString",
                        Doc(R"([{"instance_id": 1, "snapshot_max_count": "2"}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"MaxStorageSizeBytesZero",
                        Doc(R"([{"instance_id": 1, "max_storage_size_bytes": 0}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"MaxStorageSizeBytesNegative",
                        Doc(R"([{"instance_id": 1, "max_storage_size_bytes": -1}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"MaxStorageSizeBytesFractional",
                        Doc(R"([{"instance_id": 1, "max_storage_size_bytes": 1.5}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"MaxStorageSizeBytesString",
                        Doc(R"([{"instance_id": 1, "max_storage_size_bytes": "10"}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"DuplicateInstanceId",
                        Doc(R"([{"instance_id": 3}, {"instance_id": 3}])"),
                        ErrorCode::InvalidConfiguration},
        BadDocumentCase{"LaterEntryInvalidAlsoRejectsFile",
                        Doc(R"([{"instance_id": 1}, {"instance_id": 2, "snapshot_max_count": 9}])"),
                        ErrorCode::InvalidConfiguration}),
    [](const ::testing::TestParamInfo<BadDocumentCase>& info) {
        return info.param.name;
    });

////////////////////////////////////////////////////////////////////////////////
/* Lookup: load_entry() */
////////////////////////////////////////////////////////////////////////////////

using KvsConfigLookup = KvsConfigFixture;

TEST_F(KvsConfigLookup, ReturnsEntryMatchingInstanceId)
{
    SetConfigEnv(WriteConfig(Doc(R"([{"instance_id": 1, "storage_path": "./one"},
                                     {"instance_id": 7, "storage_path": "./seven"}])")));

    auto res = KvsConfigParser().load_entry(7);
    ASSERT_TRUE(res);
    ASSERT_TRUE(res.value().has_value());
    EXPECT_EQ(res.value().value().instance_id, 7u);
    EXPECT_EQ(res.value().value().storage_path, std::optional<std::string>{"./seven"});
}

TEST_F(KvsConfigLookup, NoEntryForInstanceIdIsNullopt)
{
    SetConfigEnv(WriteConfig(Doc(R"([{"instance_id": 1}])")));

    auto res = KvsConfigParser().load_entry(kTestInstanceId);
    ASSERT_TRUE(res);
    EXPECT_FALSE(res.value().has_value());
}

TEST_F(KvsConfigLookup, NoConfigurationAppliesIsNullopt)
{
    if (ExpectedFallbackPath().has_value())
    {
        GTEST_SKIP() << "binary-relative fallback configuration exists on this machine";
    }
    auto res = KvsConfigParser().load_entry(kTestInstanceId);
    ASSERT_TRUE(res);
    EXPECT_FALSE(res.value().has_value());
}

TEST_F(KvsConfigLookup, MissingExplicitTargetPropagatesFileNotFound)
{
    SetConfigEnv((std::filesystem::path(base_dir_) / "missing.json").string());

    auto res = KvsConfigParser().load_entry(kTestInstanceId);
    ExpectErrorCode(res, ErrorCode::FileNotFound);
}

TEST_F(KvsConfigLookup, InvalidFileFailsEvenForUnrelatedInstance)
{
    /* The whole file is validated, including the entry for instance 2 which is not requested. */
    SetConfigEnv(WriteConfig(Doc(R"([{"instance_id": 1},
                                     {"instance_id": 2, "snapshot_max_count": 9}])")));

    auto res = KvsConfigParser().load_entry(1);
    ExpectErrorCode(res, ErrorCode::InvalidConfiguration);
}

TEST_F(KvsConfigLookup, MalformedJsonPropagatesJsonParserError)
{
    SetConfigEnv(WriteConfig("{ not json"));

    auto res = KvsConfigParser().load_entry(kTestInstanceId);
    ExpectErrorCode(res, ErrorCode::JsonParserError);
}

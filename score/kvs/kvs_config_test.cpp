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

/* Unit tests for the configuration-by-file builder merge: the precedence (file > builder setter >
 * built-in default) applied by KvsBuilder::build(). Parser tests live in kvs_config_parser_test.cpp.
 * Design reference: others/config_by_file.md, section "As implemented (issue #411)". */

#include "kvs_config_general_test.hpp"

#include "score/kvs/kvs.hpp"
#include "score/kvs/kvsbuilder.hpp"
#include "score/kvs/kvsvalue.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <variant>

using namespace score::mw::per::kvs;
using score::mw::per::kvs::test::Doc;
using score::mw::per::kvs::test::ExpectErrorCode;
using score::mw::per::kvs::test::kFileSnapshotMaxCount;
using score::mw::per::kvs::test::kTestInstanceId;
using score::mw::per::kvs::test::KvsConfigFixture;
using score::mw::per::kvs::test::TestInstanceDoc;

namespace
{

/* `"storage_path": "<dir>"` member of a configuration entry. */
std::string StoragePath(const std::string& dir)
{
    return R"("storage_path": ")" + dir + R"(")";
}

/* Name of the `_0` KVS file of `kTestInstanceId`. */
std::string TestInstanceKvsFileName()
{
    return "kvs_" + std::to_string(kTestInstanceId) + "_0.json";
}

/* Full path of the `_0` KVS file of an opened instance. */
std::string CurrentKvsFile(const Kvs& kvs)
{
    return std::string(kvs.get_kvs_filename(SnapshotId{0}).value().Native());
}

bool IsInside(const std::string& file, const std::string& dir)
{
    return file.rfind(dir + "/", 0) == 0;
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
/* Builder: merge and precedence (file entry > builder setter > built-in default) */
////////////////////////////////////////////////////////////////////////////////

using KvsConfigBuilder = KvsConfigFixture;

TEST_F(KvsConfigBuilder, NoEntryForInstanceKeepsBuilderAndDefaultSettings)
{
    /* The configuration names only another instance, so the code configuration applies unchanged. */
    SetConfigEnv(WriteConfig(Doc(R"([{"instance_id": 99, "snapshot_max_count": 0}])")));
    const std::string dir = SubDir("data");

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(dir)).snapshot_max_count(1);
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(kvs.value().snapshot_max_count(), 1u);
    EXPECT_TRUE(kvs.value().set_value("k", KvsValue("v")));
    EXPECT_TRUE(kvs.value().flush());
    EXPECT_TRUE(IsInside(CurrentKvsFile(kvs.value()), dir));
}

TEST_F(KvsConfigBuilder, NoEntryWithoutSettingsUsesBuiltInDefaults)
{
    SetConfigEnv(WriteConfig(Doc(R"([{"instance_id": 99}])")));
    const std::string dir = SubDir("data");

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(dir));
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(kvs.value().snapshot_max_count(), static_cast<std::size_t>(KVS_DEFAULT_MAX_SNAPSHOTS));
    EXPECT_EQ(kvs.value().snapshot_max_count(), 3u);
    EXPECT_TRUE(kvs.value().set_value("k", KvsValue("v")));
    EXPECT_TRUE(kvs.value().flush());
}

TEST_F(KvsConfigBuilder, FileEntryOverridesEveryBuilderSettingItSpecifies)
{
    const std::string code_dir = SubDir("code_dir");
    const std::string file_dir = SubDir("file_dir");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(file_dir) + R"(, "snapshot_max_count": 2)")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(code_dir)).snapshot_max_count(1);
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);

    EXPECT_EQ(kvs.value().snapshot_max_count(), 2u);
    EXPECT_TRUE(kvs.value().set_value("k", KvsValue("v")));
    EXPECT_TRUE(kvs.value().flush());
    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(file_dir) / TestInstanceKvsFileName()));
    EXPECT_TRUE(ListDir(code_dir).empty());
}

TEST_F(KvsConfigBuilder, AbsentFileFieldsKeepBuilderSettings)
{
    const std::string code_dir = SubDir("code_dir");
    const std::string file_dir = SubDir("file_dir");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(file_dir))));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(code_dir)).snapshot_max_count(2);
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(kvs.value().snapshot_max_count(), 2u);
    ASSERT_TRUE(kvs.value().set_value("k", KvsValue("v")));
    ASSERT_TRUE(kvs.value().flush());
    EXPECT_TRUE(IsInside(CurrentKvsFile(kvs.value()), file_dir));
    EXPECT_TRUE(ListDir(code_dir).empty());
}

TEST_F(KvsConfigBuilder, FileNeedDefaultsOverridesBuilderRequired)
{
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(dir) + R"(, "need_defaults": "optional")")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.need_defaults_flag(OpenNeedDefaults::Required);
    EXPECT_TRUE(builder.build());
}

TEST_F(KvsConfigBuilder, FileNeedDefaultsRequiredOverridesBuilderOptional)
{
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(dir) + R"(, "need_defaults": "required")")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.need_defaults_flag(OpenNeedDefaults::Optional);
    ExpectErrorCode(builder.build(), ErrorCode::KvsFileReadError);
}

TEST_F(KvsConfigBuilder, FileNeedKvsRequiredFailsWithoutKvsFile)
{
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(dir) + R"(, "need_kvs": "required")")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    ExpectErrorCode(builder.build(), ErrorCode::KvsFileReadError);
}

TEST_F(KvsConfigBuilder, FileNeedKvsOptionalOverridesBuilderRequired)
{
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(dir) + R"(, "need_kvs": "optional")")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.need_kvs_flag(OpenNeedKvs::Required);
    EXPECT_TRUE(builder.build());
}

TEST_F(KvsConfigBuilder, InvalidFileFailsBuildAndWritesNothing)
{
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(R"("snapshot_max_count": 4)")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(dir));
    ExpectErrorCode(builder.build(), ErrorCode::InvalidConfiguration);
    EXPECT_TRUE(ListDir(dir).empty());
}

TEST_F(KvsConfigBuilder, MalformedJsonFailsBuild)
{
    SetConfigEnv(WriteConfig("[1,"));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    ExpectErrorCode(builder.build(), ErrorCode::JsonParserError);
}

TEST_F(KvsConfigBuilder, SetButMissingTargetFailsBuildWithFileNotFound)
{
    SetConfigEnv((std::filesystem::path(base_dir_) / "missing.json").string());

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    ExpectErrorCode(builder.build(), ErrorCode::FileNotFound);
}

TEST_F(KvsConfigBuilder, BuilderSnapshotMaxCountAboveThreeIsClamped)
{
    /* Out-of-range snapshot counts are not rejected by the builder; Kvs::open clamps them to
     * KVS_MAX_SNAPSHOTS_UPPER_BOUND and reports a warning. */
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.dir(std::string(dir)).snapshot_max_count(4).build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(kvs.value().snapshot_max_count(), KVS_MAX_SNAPSHOTS_UPPER_BOUND);
}

TEST_F(KvsConfigBuilder, BuilderSnapshotMaxCountZeroAndThreeAreAccepted)
{
    const std::string dir = SubDir("data");
    KvsBuilder zero{InstanceId{kTestInstanceId}};
    zero.dir(std::string(dir)).snapshot_max_count(0);
    auto kvs_zero = zero.build();
    ASSERT_TRUE(kvs_zero);
    EXPECT_EQ(kvs_zero.value().snapshot_max_count(), 0u);

    KvsBuilder three{InstanceId{kTestInstanceId + 1}};
    three.dir(std::string(dir)).snapshot_max_count(3);
    auto kvs_three = three.build();
    ASSERT_TRUE(kvs_three);
    EXPECT_EQ(kvs_three.value().snapshot_max_count(), 3u);
}

TEST_F(KvsConfigBuilder, BuildTwiceOnSameBuilderKeepsCodeDirectory)
{
    /* Regression: build() used to move the builder's directory string, so a second build() failed
     * or silently used the wrong directory. */
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(dir));

    auto first = builder.build();
    ASSERT_TRUE(first);
    ASSERT_TRUE(first.value().set_value("k", KvsValue("persisted")));
    ASSERT_TRUE(first.value().flush());

    auto second = builder.build();
    ASSERT_TRUE(second);
    EXPECT_TRUE(IsInside(CurrentKvsFile(second.value()), dir));
    auto value = second.value().get_value("k");
    ASSERT_TRUE(value);
    EXPECT_EQ(std::get<std::string>(value.value().getValue()), "persisted");
}

TEST_F(KvsConfigBuilder, BuildTwiceOnSameBuilderKeepsFileDirectoryOverride)
{
    const std::string code_dir = SubDir("code_dir");
    const std::string file_dir = SubDir("file_dir");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(file_dir))));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(code_dir));

    auto first = builder.build();
    ASSERT_TRUE(first);
    ASSERT_TRUE(first.value().set_value("k", KvsValue("v")));
    ASSERT_TRUE(first.value().flush());
    auto second = builder.build();
    ASSERT_TRUE(second);
    EXPECT_TRUE(IsInside(CurrentKvsFile(first.value()), file_dir));
    EXPECT_TRUE(IsInside(CurrentKvsFile(second.value()), file_dir));
    EXPECT_TRUE(ListDir(code_dir).empty());
}

TEST_F(KvsConfigBuilder, BuilderReadOnlyFlagRejectsSetValueAndFlush)
{
    const std::string dir = SubDir("data");
    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(dir)).read_only_flag(true);
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);

    ExpectErrorCode(kvs.value().set_value("k", KvsValue("v")), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().flush(), ErrorCode::ReadOnlyViolation);
}

TEST_F(KvsConfigBuilder, FileAccessModeReadOnlyAppliesWithoutBuilderCall)
{
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(dir) + R"(, "access_mode": "read_only")")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);

    ExpectErrorCode(kvs.value().set_value("k", KvsValue("v")), ErrorCode::ReadOnlyViolation);
    ExpectErrorCode(kvs.value().flush(), ErrorCode::ReadOnlyViolation);
}

TEST_F(KvsConfigBuilder, FileAccessModeReadWriteAllowsFlush)
{
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(dir) + R"(, "access_mode": "read_write")")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value().set_value("k", KvsValue("v")));
    EXPECT_TRUE(kvs.value().flush());
    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(dir) / TestInstanceKvsFileName()));
}

TEST_F(KvsConfigBuilder, FileAccessModeReadWriteOverridesBuilderReadOnlyFlag)
{
    /* The file entry takes precedence over the builder setter, in both directions. */
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(dir) + R"(, "access_mode": "read_write")")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.read_only_flag(true);
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value().set_value("k", KvsValue("v")));
    EXPECT_TRUE(kvs.value().flush());
    EXPECT_TRUE(std::filesystem::exists(std::filesystem::path(dir) / TestInstanceKvsFileName()));
}

TEST_F(KvsConfigBuilder, FileAccessModeReadOnlyOverridesBuilderReadWrite)
{
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(TestInstanceDoc(StoragePath(dir) + R"(, "access_mode": "read_only")")));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.read_only_flag(false);
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);

    ExpectErrorCode(kvs.value().set_value("k", KvsValue("v")), ErrorCode::ReadOnlyViolation);
}

TEST_F(KvsConfigBuilder, FileMaxStorageSizeBytesIsEnforcedByOpenedKvs)
{
    /* The storage limit is only observable through the public API via calculate_potential_size(),
     * which reports OutOfStorageSpace when the projected serialized size exceeds the limit. */
    constexpr std::size_t kFileLimitBytes{512U};
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(
        TestInstanceDoc(StoragePath(dir) + R"(, "max_storage_size_bytes": )" + std::to_string(kFileLimitBytes))));

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value().set_value("small", KvsValue("v")));
    EXPECT_TRUE(kvs.value().calculate_potential_size());

    ASSERT_TRUE(kvs.value().set_value("big", KvsValue(std::string(2U * kFileLimitBytes, 'x').c_str())));
    ExpectErrorCode(kvs.value().calculate_potential_size(), ErrorCode::OutOfStorageSpace);
}

TEST_F(KvsConfigBuilder, FileMaxStorageSizeBytesDoesNotLeakToOtherInstances)
{
    /* Control for the test above: an instance without a file entry has no storage limit. */
    constexpr std::size_t kFileLimitBytes{512U};
    const std::string dir = SubDir("data");
    SetConfigEnv(WriteConfig(
        TestInstanceDoc(StoragePath(dir) + R"(, "max_storage_size_bytes": )" + std::to_string(kFileLimitBytes))));

    KvsBuilder builder{InstanceId{kTestInstanceId + 1}};
    builder.dir(std::string(dir));
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);

    ASSERT_TRUE(kvs.value().set_value("big", KvsValue(std::string(2U * kFileLimitBytes, 'x').c_str())));
    EXPECT_TRUE(kvs.value().calculate_potential_size());
}

TEST_F(KvsConfigBuilder, DirectoryEnvVarDrivesBuildEndToEnd)
{
    /* STORAGE_CONFIG_PATH names a directory: the document inside it must be discovered by build(). */
    const std::string cfg_home = SubDir("cfg_home");
    const std::string file_dir = SubDir("file_dir");
    const std::string code_dir = SubDir("code_dir");
    WriteFile(
        "cfg_home/" + std::string(kKvsConfigFileName),
        TestInstanceDoc(StoragePath(file_dir) + R"(, "snapshot_max_count": )" + std::to_string(kFileSnapshotMaxCount)));
    SetConfigEnv(cfg_home);

    KvsBuilder builder{InstanceId{kTestInstanceId}};
    builder.dir(std::string(code_dir));
    auto kvs = builder.build();
    ASSERT_TRUE(kvs);
    EXPECT_EQ(kvs.value().snapshot_max_count(), kFileSnapshotMaxCount);
    ASSERT_TRUE(kvs.value().set_value("k", KvsValue("v")));
    ASSERT_TRUE(kvs.value().flush());
    EXPECT_TRUE(IsInside(CurrentKvsFile(kvs.value()), file_dir));
    EXPECT_TRUE(ListDir(code_dir).empty());
}

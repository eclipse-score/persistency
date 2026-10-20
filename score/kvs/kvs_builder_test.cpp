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
void remove_file(const std::string& path)
{
    score::cpp::ignore = score::filesystem::FilesystemFactory{}.CreateInstance().standard->Remove(path);
}
}  // namespace

TEST(kvs_kvsbuilder, kvsbuilder_build_ignored)
{
    // Both files are unparsable, so reading either of them would fail with kJsonParserError.
    PrepareEnvironment("./kvsbuilder/", "kvs_", kInstance, "{ invalid json }", "{ invalid json }");

    KvsBuilder builder{kInstanceId};
    builder.Snapshot(SnapshotId{0ul});
    builder.Defaults(KvsDefaults::kIgnored);
    builder.KvsLoad(KvsLoad::kIgnored);
    builder.WorkingDir("./kvsbuilder/");
    builder.SnapshotMaxCount(0U);

    auto result_build{builder.Build()};
    ASSERT_TRUE(result_build);
    auto kvs{std::move(result_build.value())};
    auto keys{kvs.GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_TRUE(keys.value().empty());

    // The file name after Flush() shows the instance ID and directory used to build the KVS.
    ASSERT_TRUE(kvs.SetValue("k", KvsValue(1)));
    ASSERT_TRUE(kvs.Flush());
    auto filename{kvs.KvsFilePath(SnapshotId{0ul})};
    ASSERT_TRUE(filename);
    EXPECT_EQ(filename.value().CStr(), "./kvsbuilder/kvs_" + std::to_string(kInstanceId.id) + "_0.json");

    CleanupEnvironment("./kvsbuilder/");
}

TEST(kvs_kvsbuilder, kvsbuilder_build)
{
    // This test also checks KVS loading through KvsBuilder.
    KvsBuilder builder{kInstanceId};
    builder.Snapshot(SnapshotId{23ul});
    builder.Defaults(KvsDefaults::kRequired);
    builder.KvsLoad(KvsLoad::kRequired);
    builder.WorkingDir("./kvsbuilder/");

    // Check that KvsDefaults::kRequired and KvsLoad::kRequired are passed correctly.
    // Build() should return an error, since the files are not available.
    auto result_build{builder.Build()};
    ASSERT_FALSE(result_build);
    // This error occurs in OpenJson() and is passed through Build().
    EXPECT_EQ(static_cast<ErrorCode>(*result_build.error()), ErrorCode::kKvsFileReadError);

    // Now the missing KVS file causes the error.
    builder.Defaults(KvsDefaults::kOptional);
    result_build = builder.Build();
    ASSERT_FALSE(result_build);
    EXPECT_EQ(static_cast<ErrorCode>(*result_build.error()), ErrorCode::kKvsFileReadError);

    builder.KvsLoad(KvsLoad::kOptional);
    result_build = builder.Build();
    EXPECT_TRUE(result_build);
    CleanupEnvironment();

    // Snapshot 23 only loads if the builder passes the snapshot ID on.
    const std::string snapshot_prefix{"./kvsbuilder/kvs_" + std::to_string(kInstance) + "_23"};
    PrepareEnvironment("./kvsbuilder/", "kvs_", kInstance, kDefaultJson, kKvsJson, kKvsJson);
    Store(
        snapshot_prefix + ".json", snapshot_prefix + ".hash", R"({"t": "obj", "v": {"snap": {"t": "i32", "v": 23}}})");
    builder.Defaults(KvsDefaults::kRequired);
    builder.KvsLoad(KvsLoad::kRequired);
    result_build = builder.Build();
    ASSERT_TRUE(result_build);
    auto snapshot_value{result_build.value().GetValue("snap")};
    ASSERT_TRUE(snapshot_value);
    ASSERT_EQ(snapshot_value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(snapshot_value.value().GetValue()), 23);
    CleanupEnvironment();

    PrepareEnvironment("./kvsbuilder/", "kvs_", kInstance, kDefaultJson, kKvsJson, kKvsJson);
    // A builder with default settings loads snapshot 0.
    KvsBuilder default_builder{kInstanceId};
    default_builder.WorkingDir("./kvsbuilder/");
    auto default_build{default_builder.Build()};
    ASSERT_TRUE(default_build);
    auto kvs_value{default_build.value().GetValue("kvs")};
    ASSERT_TRUE(kvs_value);
    ASSERT_EQ(kvs_value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(kvs_value.value().GetValue()), 2);

    CleanupEnvironment("./kvsbuilder/");
}

TEST(kvs_kvsbuilder, kvsbuilder_directory_check)
{
    // Test the KvsBuilder with all configurations for the current working directory.
    const std::string expected_prefix{"./kvs_" + std::to_string(kInstanceId.id) + "_0"};
    for (const char* dir : {"", "./", "."})
    {
        CleanupEnvironment();

        SCOPED_TRACE(dir);
        KvsBuilder builder{kInstanceId};
        builder.WorkingDir(dir);
        // No rotation, so the runs leave no _1 files behind.
        builder.SnapshotMaxCount(0U);
        auto result_build{builder.Build()};
        ASSERT_TRUE(result_build);
        auto kvs{std::move(result_build.value())};
        ASSERT_TRUE(kvs.SetValue("k", KvsValue(1)));
        ASSERT_TRUE(kvs.Flush());

        auto filename{kvs.KvsFilePath(SnapshotId{0ul})};
        remove_file(expected_prefix + ".json");
        remove_file(expected_prefix + ".hash");
        ASSERT_TRUE(filename);
        EXPECT_EQ(filename.value().CStr(), expected_prefix + ".json");
    }

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_snapshot_max_count_default)
{
    PrepareEnvironment();

    // comp_req__kvs__snapshot_max_num_cfg: an unconfigured builder must keep the compile-time default.
    // Existing users then observe no behavior change.
    KvsBuilder builder{kInstanceId};
    builder.WorkingDir(std::string(kDataDir));
    auto result_build{builder.Build()};
    ASSERT_TRUE(result_build);
    EXPECT_EQ(result_build.value().SnapshotMaxCount(), kDefaultMaxSnapshots);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_snapshot_max_count_configured)
{
    PrepareEnvironment();

    // comp_req__kvs__snapshot_max_num_cfg: the configured value must reach the built KVS instance.
    // Without the plumbing through Build() the setter would be silently ignored.
    // Inside the allowed range, and not the default.
    const std::size_t configured_max{2U};

    KvsBuilder builder{kInstanceId};
    builder.SnapshotMaxCount(configured_max);

    // The setter must be chainable like every other builder option.
    EXPECT_EQ(&builder.SnapshotMaxCount(configured_max), &builder);

    builder.WorkingDir(std::string(kDataDir));
    auto result_build{builder.Build()};
    ASSERT_TRUE(result_build);
    EXPECT_EQ(result_build.value().SnapshotMaxCount(), configured_max);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_out_of_range_instance_id)
{
    PrepareEnvironment();

    // Instance IDs are indexed into the registry array, so the first invalid ID is kMaxInstanceCount.
    auto result_build{KvsBuilder{InstanceId{kMaxInstanceCount}}.WorkingDir(std::string(kDataDir)).Build()};
    ASSERT_FALSE(result_build);
    EXPECT_EQ(result_build.error(), ErrorCode::kInvalidInstanceId);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_mismatch_snapshot_id)
{
    PrepareEnvironment();

    auto first{KvsBuilder{kInstanceId}
                   .Snapshot(SnapshotId{0ul})
                   .Defaults(KvsDefaults::kIgnored)
                   .KvsLoad(KvsLoad::kIgnored)
                   .WorkingDir(std::string(kDataDir))
                   .Build()};
    ASSERT_TRUE(first);

    // Same instance, different snapshot ID: the existing entry must not be silently reused.
    auto second{KvsBuilder{kInstanceId}
                    .Snapshot(SnapshotId{1ul})
                    .Defaults(KvsDefaults::kIgnored)
                    .KvsLoad(KvsLoad::kIgnored)
                    .WorkingDir(std::string(kDataDir))
                    .Build()};
    ASSERT_FALSE(second);
    EXPECT_EQ(second.error(), ErrorCode::kInstanceParametersMismatch);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_mismatch_max_storage_bytes)
{
    PrepareEnvironment();

    auto first{KvsBuilder{kInstanceId}
                   .Defaults(KvsDefaults::kIgnored)
                   .KvsLoad(KvsLoad::kIgnored)
                   .WorkingDir(std::string(kDataDir))
                   .MaxStorageBytes(std::optional<std::size_t>{1024U})
                   .Build()};
    ASSERT_TRUE(first);

    // Identical storage limit: reuse the existing entry.
    auto same{KvsBuilder{kInstanceId}
                  .Defaults(KvsDefaults::kIgnored)
                  .KvsLoad(KvsLoad::kIgnored)
                  .WorkingDir(std::string(kDataDir))
                  .MaxStorageBytes(std::optional<std::size_t>{1024U})
                  .Build()};
    EXPECT_TRUE(same);

    // Different storage limit, including removing the limit entirely: must be rejected.
    auto different{KvsBuilder{kInstanceId}
                       .Defaults(KvsDefaults::kIgnored)
                       .KvsLoad(KvsLoad::kIgnored)
                       .WorkingDir(std::string(kDataDir))
                       .MaxStorageBytes(std::optional<std::size_t>{2048U})
                       .Build()};
    ASSERT_FALSE(different);
    EXPECT_EQ(different.error(), ErrorCode::kInstanceParametersMismatch);

    auto unlimited{KvsBuilder{kInstanceId}
                       .Defaults(KvsDefaults::kIgnored)
                       .KvsLoad(KvsLoad::kIgnored)
                       .WorkingDir(std::string(kDataDir))
                       .MaxStorageBytes(std::nullopt)
                       .Build()};
    ASSERT_FALSE(unlimited);
    EXPECT_EQ(unlimited.error(), ErrorCode::kInstanceParametersMismatch);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_mismatch_working_dir)
{
    PrepareEnvironment();

    auto first{KvsBuilder{kInstanceId}
                   .Defaults(KvsDefaults::kIgnored)
                   .KvsLoad(KvsLoad::kIgnored)
                   .WorkingDir(std::string(kDataDir))
                   .Build()};
    ASSERT_TRUE(first);

    // Same instance ID resolves to a different filename prefix: must be rejected.
    auto other_dir{KvsBuilder{kInstanceId}
                       .Defaults(KvsDefaults::kIgnored)
                       .KvsLoad(KvsLoad::kIgnored)
                       .WorkingDir(std::string(kDataDir) + "_other")
                       .Build()};
    ASSERT_FALSE(other_dir);
    EXPECT_EQ(other_dir.error(), ErrorCode::kInstanceParametersMismatch);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_clamped_max_count_matches_existing_entry)
{
    PrepareEnvironment();

    // A request above the upper bound is stored clamped to kMaxSnapshotsUpperBound.
    auto first{KvsBuilder{kInstanceId}
                   .Defaults(KvsDefaults::kIgnored)
                   .KvsLoad(KvsLoad::kIgnored)
                   .WorkingDir(std::string(kDataDir))
                   .SnapshotMaxCount(100U)
                   .Build()};
    ASSERT_TRUE(first);
    EXPECT_EQ(first.value().SnapshotMaxCount(), kMaxSnapshotsUpperBound);

    // Comparing against the clamped value, not the raw request: rebuilding with the bound must match.
    auto at_bound{KvsBuilder{kInstanceId}
                      .Defaults(KvsDefaults::kIgnored)
                      .KvsLoad(KvsLoad::kIgnored)
                      .WorkingDir(std::string(kDataDir))
                      .SnapshotMaxCount(kMaxSnapshotsUpperBound)
                      .Build()};
    ASSERT_TRUE(at_bound);
    EXPECT_EQ(at_bound.value().SnapshotMaxCount(), kMaxSnapshotsUpperBound);

    // A genuinely different count is still a mismatch.
    auto below_bound{KvsBuilder{kInstanceId}
                         .Defaults(KvsDefaults::kIgnored)
                         .KvsLoad(KvsLoad::kIgnored)
                         .WorkingDir(std::string(kDataDir))
                         .SnapshotMaxCount(kMaxSnapshotsUpperBound - 1U)
                         .Build()};
    ASSERT_FALSE(below_bound);
    EXPECT_EQ(below_bound.error(), ErrorCode::kInstanceParametersMismatch);

    CleanupEnvironment();
}

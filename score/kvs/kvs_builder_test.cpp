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

#include <chrono>
#include <future>
#include <mutex>

using namespace score::kvs;

namespace
{
void RemoveFile(const std::string& path)
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
    auto& kvs{*result_build.value()};
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
    auto snapshot_value{result_build.value()->GetValue("snap")};
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
    auto kvs_value{default_build.value()->GetValue("kvs")};
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
        auto& kvs{*result_build.value()};
        ASSERT_TRUE(kvs.SetValue("k", KvsValue(1)));
        ASSERT_TRUE(kvs.Flush());

        auto filename{kvs.KvsFilePath(SnapshotId{0ul})};
        RemoveFile(expected_prefix + ".json");
        RemoveFile(expected_prefix + ".hash");
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
    EXPECT_EQ(result_build.value()->SnapshotMaxCount(), kDefaultMaxSnapshots);

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
    EXPECT_EQ(result_build.value()->SnapshotMaxCount(), configured_max);

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
    EXPECT_EQ(first.value()->SnapshotMaxCount(), kMaxSnapshotsUpperBound);

    // Comparing against the clamped value, not the raw request: rebuilding with the bound must match.
    auto at_bound{KvsBuilder{kInstanceId}
                      .Defaults(KvsDefaults::kIgnored)
                      .KvsLoad(KvsLoad::kIgnored)
                      .WorkingDir(std::string(kDataDir))
                      .SnapshotMaxCount(kMaxSnapshotsUpperBound)
                      .Build()};
    ASSERT_TRUE(at_bound);
    EXPECT_EQ(at_bound.value()->SnapshotMaxCount(), kMaxSnapshotsUpperBound);

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

TEST(kvs_kvsbuilder, kvsbuilder_last_valid_instance_id)
{
    PrepareEnvironment();

    auto result_build{KvsBuilder{InstanceId{kMaxInstanceCount - 1U}}
                          .Defaults(KvsDefaults::kIgnored)
                          .KvsLoad(KvsLoad::kIgnored)
                          .WorkingDir(std::string(kDataDir))
                          .Build()};
    EXPECT_TRUE(result_build);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_failed_build_leaves_no_entry)
{
    const std::string dir{"./kvsbuilder_failed_build/"};
    CleanupEnvironment(dir);

    // The required defaults file is missing, so the first build fails.
    auto failed{
        KvsBuilder{kInstanceId}.Defaults(KvsDefaults::kRequired).KvsLoad(KvsLoad::kIgnored).WorkingDir(dir).Build()};
    ASSERT_FALSE(failed);
    EXPECT_EQ(KvsInstanceRegistry::GetInstance().array.at(kInstance), nullptr);

    // A leftover entry would reject different options with kInstanceParametersMismatch.
    auto retried{
        KvsBuilder{kInstanceId}.Defaults(KvsDefaults::kIgnored).KvsLoad(KvsLoad::kIgnored).WorkingDir(dir).Build()};
    EXPECT_TRUE(retried);

    CleanupEnvironment(dir);
}

TEST(kvs_kvsbuilder, kvsbuilder_mismatch_load_policy)
{
    PrepareEnvironment();

    auto first{KvsBuilder{kInstanceId}
                   .Defaults(KvsDefaults::kIgnored)
                   .KvsLoad(KvsLoad::kIgnored)
                   .WorkingDir(std::string(kDataDir))
                   .Build()};
    ASSERT_TRUE(first);

    // The shared state was built without loading any file, so a later build must not report the files as loaded.
    auto required_defaults{KvsBuilder{kInstanceId}
                               .Defaults(KvsDefaults::kRequired)
                               .KvsLoad(KvsLoad::kIgnored)
                               .WorkingDir(std::string(kDataDir))
                               .Build()};
    ASSERT_FALSE(required_defaults);
    EXPECT_EQ(required_defaults.error(), ErrorCode::kInstanceParametersMismatch);

    auto required_kvs{KvsBuilder{kInstanceId}
                          .Defaults(KvsDefaults::kIgnored)
                          .KvsLoad(KvsLoad::kRequired)
                          .WorkingDir(std::string(kDataDir))
                          .Build()};
    ASSERT_FALSE(required_kvs);
    EXPECT_EQ(required_kvs.error(), ErrorCode::kInstanceParametersMismatch);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, unregister_removes_only_matching_instance)
{
    RecordProperty("PartiallyVerifies", "comp_req__kvs__multi_instance");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "design-analysis");
    RecordProperty("Description",
                   "Check unregistering an instance leaves other instances intact and allows an independent reopen.");
    PrepareEnvironment();

    KvsBuilder builder{kInstanceId};
    builder.WorkingDir(kDataDir).Defaults(KvsDefaults::kRequired).KvsLoad(KvsLoad::kRequired);
    auto first{builder.Build()};
    ASSERT_TRUE(first);
    auto old_handle{first.value()};
    ASSERT_TRUE(old_handle->SetValue("pending", KvsValue(int32_t{7})));

    KvsBuilder other_builder{InstanceId{kInstanceId.id + 1U}};
    other_builder.WorkingDir(kDataDir).Defaults(KvsDefaults::kIgnored).KvsLoad(KvsLoad::kIgnored);
    auto other{other_builder.Build()};
    ASSERT_TRUE(other);
    ASSERT_TRUE(other.value()->SetValue("other", KvsValue(int32_t{8})));

    auto invalid_id{Unregister(InstanceId{kMaxInstanceCount})};
    ASSERT_FALSE(invalid_id);
    EXPECT_EQ(invalid_id.error(), ErrorCode::kInvalidInstanceId);

    auto removed{Unregister(kInstanceId)};
    ASSERT_TRUE(removed);
    auto already_removed{Unregister(kInstanceId)};
    ASSERT_FALSE(already_removed);
    EXPECT_EQ(already_removed.error(), ErrorCode::kInvalidInstanceId);
    builder.Defaults(KvsDefaults::kIgnored);
    auto reopened{builder.Build()};
    ASSERT_TRUE(reopened);
    EXPECT_NE(old_handle, reopened.value());

    auto pending{old_handle->GetValue("pending")};
    ASSERT_TRUE(pending);
    EXPECT_EQ(std::get<int32_t>(pending.value().GetValue()), 7);
    auto reopened_pending{reopened.value()->KeyExists("pending")};
    ASSERT_TRUE(reopened_pending);
    EXPECT_FALSE(reopened_pending.value());
    auto persisted{reopened.value()->GetValue("kvs")};
    ASSERT_TRUE(persisted);
    EXPECT_EQ(std::get<int32_t>(persisted.value().GetValue()), 2);

    auto reopened_again{builder.Build()};
    ASSERT_TRUE(reopened_again);
    EXPECT_EQ(reopened.value(), reopened_again.value());
    auto other_again{other_builder.Build()};
    ASSERT_TRUE(other_again);
    EXPECT_EQ(other.value(), other_again.value());
    auto other_value{other_again.value()->GetValue("other")};
    ASSERT_TRUE(other_value);
    EXPECT_EQ(std::get<int32_t>(other_value.value().GetValue()), 8);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, concurrent_unregister_removes_instance_once)
{
    RecordProperty("PartiallyVerifies", "comp_req__kvs__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check concurrent unregister calls remove the registered instance only once.");
    PrepareEnvironment();

    auto opened{KvsBuilder{kInstanceId}.WorkingDir(kDataDir).Build()};
    ASSERT_TRUE(opened);
    auto unregister = []() {
        return Unregister(kInstanceId);
    };
    auto first{std::async(std::launch::async, unregister)};
    auto second{std::async(std::launch::async, unregister)};

    auto first_result{first.get()};
    auto second_result{second.get()};
    ASSERT_NE(first_result.has_value(), second_result.has_value());
    EXPECT_EQ(first_result ? second_result.error() : first_result.error(), ErrorCode::kInvalidInstanceId);
    auto reopened{KvsBuilder{kInstanceId}.WorkingDir(kDataDir).Build()};
    ASSERT_TRUE(reopened);
    EXPECT_NE(opened.value(), reopened.value());

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_concurrent_build_shares_state)
{
    RecordProperty("PartiallyVerifies", "comp_req__kvs__concurrency");
    RecordProperty("TestType", "requirements-based");
    RecordProperty("DerivationTechnique", "error-guessing");
    RecordProperty("Description", "Check concurrent builds return one registered instance and share its data.");
    PrepareEnvironment();

    const auto build = []() {
        return KvsBuilder{kInstanceId}
            .Defaults(KvsDefaults::kRequired)
            .KvsLoad(KvsLoad::kRequired)
            .WorkingDir(std::string(kDataDir))
            .Build();
    };

    auto& registry{KvsInstanceRegistry::GetInstance()};
    std::future<score::Result<std::shared_ptr<Kvs>>> first;
    std::future<score::Result<std::shared_ptr<Kvs>>> second;
    {
        // Holding the registry lock prevents either build from accessing the slot until it is released.
        std::unique_lock<std::mutex> registry_lock{registry.mutex};
        first = std::async(std::launch::async, build);
        second = std::async(std::launch::async, build);

        // Each Build() must wait for the registry lock before it loads files or touches the slot.
        EXPECT_EQ(first.wait_for(std::chrono::milliseconds{200}), std::future_status::timeout);
        EXPECT_EQ(second.wait_for(std::chrono::milliseconds{0}), std::future_status::timeout);
        EXPECT_EQ(registry.array.at(kInstance), nullptr);
    }

    auto first_result{first.get()};
    auto second_result{second.get()};
    ASSERT_TRUE(first_result);
    ASSERT_TRUE(second_result);
    EXPECT_EQ(first_result.value(), second_result.value());

    // The registry and both callers own the same instance.
    {
        std::unique_lock<std::mutex> registry_lock{registry.mutex};
        EXPECT_EQ(registry.array.at(kInstance).use_count(), 3);
    }

    // Both handles see one state: a change through one is visible through the other.
    ASSERT_TRUE(first_result.value()->SetValue("concurrent", KvsValue(int32_t{7})));
    auto value{second_result.value()->GetValue("concurrent")};
    ASSERT_TRUE(value);
    ASSERT_EQ(value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 7);

    std::weak_ptr<Kvs> retained{first_result.value()};
    first_result.value().reset();
    second_result.value().reset();
    ASSERT_FALSE(retained.expired());
    auto reopened{build()};
    ASSERT_TRUE(reopened);
    EXPECT_EQ(reopened.value(), retained.lock());

    CleanupEnvironment();
}

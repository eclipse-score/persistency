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

#include "score/kvs/kvs_configuration.h"

#include <chrono>
#include <fstream>
#include <future>
#include <iterator>
#include <mutex>
#include <string>

using namespace score::kvs;

namespace
{

void RemoveFile(const std::string& path)
{
    score::cpp::ignore = score::filesystem::FilesystemFactory{}.CreateInstance().standard->Remove(path);
}

score::Result<void> Initialize(const KvsParameters& params)
{
    KvsConfiguration configuration;
    EXPECT_TRUE(configuration.Add(params));
    return Initialize(configuration);
}

/// Initializes one instance and returns a handle to it.
score::Result<Kvs> InitializeAndGet(const KvsParameters& params)
{
    const auto initialized{Initialize(params)};
    if (!initialized)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*initialized.error()));
    }
    return Get(InstanceId{params.instance_id});
}

KvsParameters IgnoredOptions(const std::size_t instance_id, const std::string& dir = std::string{kDataDir})
{
    return KvsParametersBuilder{InstanceId{instance_id}}
        .Defaults(KvsDefaults::kIgnored)
        .KvsLoad(KvsLoad::kIgnored)
        .WorkingDir(dir)
        .Build();
}

/// Configuration document with one instance that reads no files.
std::string IgnoredConfigJson(const std::size_t instance_id, const std::string& dir)
{
    return R"({"formatVersion": 1, "kvsInstances": [{"instanceId": )" + std::to_string(instance_id) +
           R"(, "defaults": "ignored", "kvsLoad": "ignored", "workingDir": ")" + dir + R"("}]})";
}

class kvs_runtime : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        KvsInstanceStateRegistry::GetInstance().Reset();
    }

    void TearDown() override
    {
        KvsInstanceStateRegistry::GetInstance().Reset();
    }
};

}  // namespace

TEST_F(kvs_runtime, initialize_ignored)
{
    // Both files are unparsable, so reading either of them would fail with kJsonParserError.
    PrepareEnvironment("./kvsbuilder/", "kvs_", kInstance, "{ invalid json }", "{ invalid json }");

    auto result{InitializeAndGet(KvsParametersBuilder{kInstanceId}
                                     .Snapshot(SnapshotId{0ul})
                                     .Defaults(KvsDefaults::kIgnored)
                                     .KvsLoad(KvsLoad::kIgnored)
                                     .WorkingDir("./kvsbuilder/")
                                     .SnapshotMaxCount(0U)
                                     .Build())};
    ASSERT_TRUE(result);
    auto kvs{std::move(result.value())};
    auto keys{kvs.GetAllKeys()};
    ASSERT_TRUE(keys);
    EXPECT_TRUE(keys.value().empty());

    // The file name after Flush() shows the instance ID and directory used to open the KVS.
    ASSERT_TRUE(kvs.SetValue("k", KvsValue(1)));
    ASSERT_TRUE(kvs.Flush());
    auto filename{kvs.KvsFilePath(SnapshotId{0ul})};
    ASSERT_TRUE(filename);
    EXPECT_EQ(filename.value().CStr(), "./kvsbuilder/kvs_" + std::to_string(kInstanceId.id) + "_0.json");

    CleanupEnvironment("./kvsbuilder/");
}

TEST_F(kvs_runtime, initialize_load_policies_and_snapshot)
{
    KvsParametersBuilder builder{kInstanceId};
    builder.Snapshot(SnapshotId{23ul});
    builder.Defaults(KvsDefaults::kRequired);
    builder.KvsLoad(KvsLoad::kRequired);
    builder.WorkingDir("./kvsbuilder/");

    // The files are not available, so the required load policies fail the initialization.
    auto result{InitializeAndGet(builder.Build())};
    ASSERT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kKvsFileReadError);

    // Now the missing KVS file causes the error.
    builder.Defaults(KvsDefaults::kOptional);
    result = InitializeAndGet(builder.Build());
    ASSERT_FALSE(result);
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), ErrorCode::kKvsFileReadError);

    builder.KvsLoad(KvsLoad::kOptional);
    result = InitializeAndGet(builder.Build());
    EXPECT_TRUE(result);
    CleanupEnvironment();

    // Snapshot 23 only loads if the options carry the snapshot ID.
    const std::string snapshot_prefix{"./kvsbuilder/kvs_" + std::to_string(kInstance) + "_23"};
    PrepareEnvironment("./kvsbuilder/", "kvs_", kInstance, kDefaultJson, kKvsJson, kKvsJson);
    Store(
        snapshot_prefix + ".json", snapshot_prefix + ".hash", R"({"t": "obj", "v": {"snap": {"t": "i32", "v": 23}}})");
    builder.Defaults(KvsDefaults::kRequired);
    builder.KvsLoad(KvsLoad::kRequired);
    result = InitializeAndGet(builder.Build());
    ASSERT_TRUE(result);
    auto snapshot_value{result.value().GetValue("snap")};
    ASSERT_TRUE(snapshot_value);
    ASSERT_EQ(snapshot_value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(snapshot_value.value().GetValue()), 23);
    CleanupEnvironment();

    PrepareEnvironment("./kvsbuilder/", "kvs_", kInstance, kDefaultJson, kKvsJson, kKvsJson);
    // Default options load snapshot 0.
    KvsParametersBuilder default_builder{kInstanceId};
    default_builder.WorkingDir("./kvsbuilder/");
    auto default_result{InitializeAndGet(default_builder.Build())};
    ASSERT_TRUE(default_result);
    auto kvs_value{default_result.value().GetValue("kvs")};
    ASSERT_TRUE(kvs_value);
    ASSERT_EQ(kvs_value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(kvs_value.value().GetValue()), 2);

    CleanupEnvironment("./kvsbuilder/");
}

TEST_F(kvs_runtime, initialize_directory_check)
{
    // All spellings of the current working directory.
    const std::string expected_prefix{"./kvs_" + std::to_string(kInstanceId.id) + "_0"};
    for (const char* dir : {"", "./", "."})
    {
        CleanupEnvironment();

        SCOPED_TRACE(dir);
        KvsParametersBuilder builder{kInstanceId};
        builder.WorkingDir(dir);
        // No rotation, so the runs leave no _1 files behind.
        builder.SnapshotMaxCount(0U);
        auto result{InitializeAndGet(builder.Build())};
        ASSERT_TRUE(result);
        auto kvs{std::move(result.value())};
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

TEST_F(kvs_runtime, initialize_snapshot_max_count_default)
{
    PrepareEnvironment();

    // An unconfigured builder keeps the compile-time default.
    KvsParametersBuilder builder{kInstanceId};
    builder.WorkingDir(std::string(kDataDir));
    auto result{InitializeAndGet(builder.Build())};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().SnapshotMaxCount(), kDefaultMaxSnapshots);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, initialize_snapshot_max_count_configured)
{
    PrepareEnvironment();

    // The configured value must reach the opened instance.
    // Inside the allowed range, and not the default.
    const std::size_t configured_max{2U};

    KvsParametersBuilder builder{kInstanceId};
    builder.SnapshotMaxCount(configured_max);
    builder.WorkingDir(std::string(kDataDir));
    auto result{InitializeAndGet(builder.Build())};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().SnapshotMaxCount(), configured_max);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, initialize_snapshot_max_count_clamped)
{
    PrepareEnvironment();

    auto result{InitializeAndGet(KvsParametersBuilder{kInstanceId}
                                     .Defaults(KvsDefaults::kIgnored)
                                     .KvsLoad(KvsLoad::kIgnored)
                                     .WorkingDir(std::string(kDataDir))
                                     .SnapshotMaxCount(100U)
                                     .Build())};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().SnapshotMaxCount(), kMaxSnapshotsUpperBound);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, get_out_of_range_instance_id)
{
    PrepareEnvironment();
    ASSERT_TRUE(Initialize(IgnoredOptions(kInstance)));

    // Instance IDs are indexed into the registry array, so the first invalid ID is kMaxInstanceCount.
    auto result{Get(InstanceId{kMaxInstanceCount})};
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kInvalidInstanceId);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, configuration_rejects_out_of_range_instance_id)
{
    KvsConfiguration configuration;
    auto added{configuration.Add(IgnoredOptions(kMaxInstanceCount))};
    ASSERT_FALSE(added);
    EXPECT_EQ(added.error(), ErrorCode::kInvalidConfiguration);

    // Nothing was added, so the initialization has no instance to open.
    ASSERT_TRUE(Initialize(configuration));
    auto result{Get(InstanceId{kMaxInstanceCount - 1U})};
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kInstanceNotInitialized);
}

TEST_F(kvs_runtime, initialize_last_valid_instance_id)
{
    PrepareEnvironment();

    auto result{InitializeAndGet(IgnoredOptions(kMaxInstanceCount - 1U))};
    EXPECT_TRUE(result);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, initialize_then_get)
{
    PrepareEnvironment();

    KvsConfiguration configuration;
    ASSERT_TRUE(configuration.Add(KvsParametersBuilder{kInstanceId}
                                      .Defaults(KvsDefaults::kRequired)
                                      .KvsLoad(KvsLoad::kRequired)
                                      .WorkingDir(std::string(kDataDir))
                                      .Build()));
    ASSERT_TRUE(Initialize(configuration));

    auto kvs{Get(kInstanceId)};
    ASSERT_TRUE(kvs);
    auto value{kvs.value().GetValue("kvs")};
    ASSERT_TRUE(value);
    ASSERT_EQ(value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 2);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, initialize_opens_every_configured_instance)
{
    PrepareEnvironment();

    KvsConfiguration configuration;
    ASSERT_TRUE(configuration.Add(IgnoredOptions(1U)));
    ASSERT_TRUE(configuration.Add(IgnoredOptions(kInstance)));
    ASSERT_TRUE(Initialize(configuration));

    EXPECT_TRUE(Get(InstanceId{1U}));
    EXPECT_TRUE(Get(InstanceId{kInstance}));

    CleanupEnvironment();
}

TEST_F(kvs_runtime, get_before_initialize)
{
    auto result{Get(kInstanceId)};
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kInstanceNotInitialized);
}

TEST_F(kvs_runtime, get_unconfigured_instance_after_initialize)
{
    PrepareEnvironment();
    ASSERT_TRUE(Initialize(IgnoredOptions(kInstance)));

    auto result{Get(InstanceId{kInstance + 1U})};
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kInstanceNotInitialized);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, initialize_empty_configuration)
{
    const KvsConfiguration configuration;
    ASSERT_TRUE(Initialize(configuration));

    auto result{Get(kInstanceId)};
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error(), ErrorCode::kInstanceNotInitialized);

    // An empty initialization is still an initialization.
    auto second{Initialize(configuration)};
    ASSERT_FALSE(second);
    EXPECT_EQ(second.error(), ErrorCode::kAlreadyInitialized);
}

TEST_F(kvs_runtime, second_initialize_fails)
{
    PrepareEnvironment();
    ASSERT_TRUE(Initialize(IgnoredOptions(kInstance)));
    auto kvs{Get(kInstanceId)};
    ASSERT_TRUE(kvs);
    ASSERT_TRUE(kvs.value().SetValue("k", KvsValue(1)));

    auto second{Initialize(IgnoredOptions(kInstance))};
    ASSERT_FALSE(second);
    EXPECT_EQ(second.error(), ErrorCode::kAlreadyInitialized);

    // The existing state is untouched.
    EXPECT_TRUE(kvs.value().KeyExists("k").value());

    CleanupEnvironment();
}

TEST_F(kvs_runtime, failed_initialize_leaves_no_entry)
{
    const std::string dir{"./kvsbuilder_failed_initialize/"};
    CleanupEnvironment(dir);

    // The required defaults file is missing, so the initialization fails.
    auto failed{Initialize(KvsParametersBuilder{kInstanceId}
                               .Defaults(KvsDefaults::kRequired)
                               .KvsLoad(KvsLoad::kIgnored)
                               .WorkingDir(dir)
                               .Build())};
    ASSERT_FALSE(failed);
    EXPECT_EQ(static_cast<ErrorCode>(*failed.error()), ErrorCode::kKvsFileReadError);

    auto& registry{KvsInstanceStateRegistry::GetInstance()};
    std::unique_lock<std::mutex> lock{registry.mutex};
    EXPECT_EQ(registry.array.at(kInstance), nullptr);
    EXPECT_FALSE(registry.initialized);
    lock.unlock();

    CleanupEnvironment(dir);
}

TEST_F(kvs_runtime, failed_initialize_rolls_back_other_instances)
{
    const std::string dir{"./kvsbuilder_failed_initialize/"};
    CleanupEnvironment(dir);

    // Instance 1 opens, then instance 2 fails because its required defaults file is missing.
    KvsConfiguration configuration;
    ASSERT_TRUE(configuration.Add(IgnoredOptions(1U, dir)));
    ASSERT_TRUE(configuration.Add(KvsParametersBuilder{InstanceId{2U}}
                                      .Defaults(KvsDefaults::kRequired)
                                      .KvsLoad(KvsLoad::kIgnored)
                                      .WorkingDir(dir)
                                      .Build()));
    auto failed{Initialize(configuration)};
    ASSERT_FALSE(failed);

    auto first{Get(InstanceId{1U})};
    ASSERT_FALSE(first);
    EXPECT_EQ(first.error(), ErrorCode::kInstanceNotInitialized);
    auto second{Get(InstanceId{2U})};
    ASSERT_FALSE(second);
    EXPECT_EQ(second.error(), ErrorCode::kInstanceNotInitialized);

    CleanupEnvironment(dir);
}

TEST_F(kvs_runtime, initialize_retry_after_failure_succeeds)
{
    const std::string dir{"./kvsbuilder_failed_initialize/"};
    CleanupEnvironment(dir);

    const auto options{KvsParametersBuilder{kInstanceId}
                           .Defaults(KvsDefaults::kRequired)
                           .KvsLoad(KvsLoad::kIgnored)
                           .WorkingDir(dir)
                           .Build()};
    ASSERT_FALSE(Initialize(options));

    // Providing the missing file makes the same call succeed.
    PrepareEnvironment(dir, "kvs_", kInstance, kDefaultJson);
    ASSERT_TRUE(Initialize(options));
    EXPECT_TRUE(Get(kInstanceId));

    CleanupEnvironment(dir);
}

TEST_F(kvs_runtime, get_handles_share_state)
{
    PrepareEnvironment();
    ASSERT_TRUE(Initialize(IgnoredOptions(kInstance)));

    auto first{Get(kInstanceId)};
    auto second{Get(kInstanceId)};
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    // A change through one handle is visible through the other.
    ASSERT_TRUE(first.value().SetValue("shared", KvsValue(int32_t{7})));
    auto value{second.value().GetValue("shared")};
    ASSERT_TRUE(value);
    ASSERT_EQ(value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 7);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, concurrent_initialize_opens_once)
{
    PrepareEnvironment();

    const auto initialize = []() {
        return Initialize(KvsParametersBuilder{kInstanceId}
                              .Defaults(KvsDefaults::kRequired)
                              .KvsLoad(KvsLoad::kRequired)
                              .WorkingDir(std::string(kDataDir))
                              .Build());
    };

    auto& registry{KvsInstanceStateRegistry::GetInstance()};
    std::future<score::Result<void>> first;
    std::future<score::Result<void>> second;
    {
        // Holding the registry lock forces both calls to start together once it is released.
        // Without that, they would race for the registry only by chance.
        std::unique_lock<std::mutex> registry_lock{registry.mutex};
        first = std::async(std::launch::async, initialize);
        second = std::async(std::launch::async, initialize);

        // Each call must wait for the registry lock before it loads files or touches the slot.
        EXPECT_EQ(first.wait_for(std::chrono::milliseconds{200}), std::future_status::timeout);
        EXPECT_EQ(second.wait_for(std::chrono::milliseconds{0}), std::future_status::timeout);
        EXPECT_EQ(registry.array.at(kInstance), nullptr);
    }

    const auto first_result{first.get()};
    const auto second_result{second.get()};

    // Exactly one call opens the instance, the other one sees the finished initialization.
    EXPECT_NE(first_result.has_value(), second_result.has_value());
    const auto& failed{first_result.has_value() ? second_result : first_result};
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error(), ErrorCode::kAlreadyInitialized);

    {
        std::unique_lock<std::mutex> registry_lock{registry.mutex};
        EXPECT_EQ(registry.array.at(kInstance).use_count(), 1);
    }

    CleanupEnvironment();
}

TEST_F(kvs_runtime, concurrent_get_shares_state)
{
    PrepareEnvironment();
    ASSERT_TRUE(Initialize(KvsParametersBuilder{kInstanceId}
                               .Defaults(KvsDefaults::kRequired)
                               .KvsLoad(KvsLoad::kRequired)
                               .WorkingDir(std::string(kDataDir))
                               .Build()));

    auto& registry{KvsInstanceStateRegistry::GetInstance()};
    std::future<score::Result<Kvs>> first;
    std::future<score::Result<Kvs>> second;
    {
        std::unique_lock<std::mutex> registry_lock{registry.mutex};
        first = std::async(std::launch::async, []() {
            return Get(kInstanceId);
        });
        second = std::async(std::launch::async, []() {
            return Get(kInstanceId);
        });

        // Each call must wait for the registry lock.
        EXPECT_EQ(first.wait_for(std::chrono::milliseconds{200}), std::future_status::timeout);
        EXPECT_EQ(second.wait_for(std::chrono::milliseconds{0}), std::future_status::timeout);
    }

    auto first_result{first.get()};
    auto second_result{second.get()};
    ASSERT_TRUE(first_result);
    ASSERT_TRUE(second_result);

    // The registry owns the entry together with both handles.
    {
        std::unique_lock<std::mutex> registry_lock{registry.mutex};
        EXPECT_EQ(registry.array.at(kInstance).use_count(), 3);
    }

    // Both handles see one state: a change through one is visible through the other.
    ASSERT_TRUE(first_result.value().SetValue("concurrent", KvsValue(int32_t{7})));
    auto value{second_result.value().GetValue("concurrent")};
    ASSERT_TRUE(value);
    ASSERT_EQ(value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 7);

    CleanupEnvironment();
}

TEST_F(kvs_runtime, initialize_from_example_file)
{
    // The example stores its data below ./data/, which this test moves to its own directory.
    const std::string dir{"./kvs_runtime_example/"};
    std::ifstream example_stream{"score/kvs/config/kvs_config.example.json"};
    ASSERT_TRUE(example_stream.is_open());
    std::string example{std::istreambuf_iterator<char>(example_stream), std::istreambuf_iterator<char>()};
    const std::string data_dir{"./data/"};
    for (auto pos{example.find(data_dir)}; pos != std::string::npos; pos = example.find(data_dir, pos))
    {
        example.replace(pos, data_dir.size(), dir);
    }

    filesystem_test::CreateDirectories(dir + "settings/");
    const std::string config_path{dir + "kvs_config.json"};
    std::ofstream{config_path} << example;

    // Instance 0 requires its default values.
    Store(dir + "settings/kvs_0_default.json", dir + "settings/kvs_0_default.hash", kDefaultJson);

    const auto configuration{KvsConfiguration::Load(config_path)};
    ASSERT_TRUE(configuration);
    ASSERT_TRUE(Initialize(configuration.value()));

    for (const std::size_t id : {0U, 1U, 4U})
    {
        SCOPED_TRACE(id);
        EXPECT_TRUE(Get(InstanceId{id}));
    }
    const auto unconfigured{Get(InstanceId{2U})};
    ASSERT_FALSE(unconfigured);
    EXPECT_EQ(unconfigured.error(), ErrorCode::kInstanceNotInitialized);

    // The default value comes from the file that the configuration requires.
    auto settings{Get(InstanceId{0U})};
    ASSERT_TRUE(settings);
    auto value{settings.value().GetDefaultValue("default")};
    ASSERT_TRUE(value);
    ASSERT_EQ(value.value().GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(value.value().GetValue()), 5);

    // A change through one handle is visible through a handle from a later Get().
    ASSERT_TRUE(settings.value().SetValue("example", KvsValue(int32_t{3})));
    auto later{Get(InstanceId{0U})};
    ASSERT_TRUE(later);
    auto shared{later.value().GetValue("example")};
    ASSERT_TRUE(shared);
    EXPECT_EQ(std::get<int32_t>(shared.value().GetValue()), 3);

    filesystem_test::RemoveAll(dir);
}

TEST_F(kvs_runtime, initialize_from_path)
{
    const std::string dir{"./kvs_runtime_path/"};
    filesystem_test::CreateDirectories(dir);
    const std::string path{dir + "kvs_config.json"};
    std::ofstream{path} << IgnoredConfigJson(1U, dir);

    ASSERT_TRUE(Initialize(path));
    EXPECT_TRUE(Get(InstanceId{1U}));
    const auto unconfigured{Get(InstanceId{2U})};
    ASSERT_FALSE(unconfigured);
    EXPECT_EQ(unconfigured.error(), ErrorCode::kInstanceNotInitialized);

    filesystem_test::RemoveAll(dir);
}

TEST_F(kvs_runtime, initialize_from_missing_path_is_read_error)
{
    const std::string dir{"./kvs_runtime_missing_path/"};
    filesystem_test::RemoveAll(dir);
    const std::string path{dir + "kvs_config.json"};

    const auto failed{Initialize(path)};
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error(), ErrorCode::kConfigFileReadError);
    {
        auto& registry{KvsInstanceStateRegistry::GetInstance()};
        const std::unique_lock<std::mutex> lock{registry.mutex};
        EXPECT_FALSE(registry.initialized);
    }

    // Providing the missing file makes the same call succeed.
    filesystem_test::CreateDirectories(dir);
    std::ofstream{path} << IgnoredConfigJson(1U, dir);
    ASSERT_TRUE(Initialize(path));
    EXPECT_TRUE(Get(InstanceId{1U}));

    filesystem_test::RemoveAll(dir);
}

TEST_F(kvs_runtime, initialize_from_invalid_file_is_invalid_configuration)
{
    const std::string dir{"./kvs_runtime_invalid_file/"};
    filesystem_test::CreateDirectories(dir);
    const std::string path{dir + "kvs_config.json"};
    std::ofstream{path} << "{";

    const auto failed{Initialize(path)};
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error(), ErrorCode::kInvalidConfiguration);

    filesystem_test::RemoveAll(dir);
}

TEST_F(kvs_runtime, initialize_from_default_path)
{
    // The default path is relative to the working directory, which other tests share.
    const std::string dir{"./etc/"};
    filesystem_test::CreateDirectories(dir);
    std::ofstream{dir + "kvs_config.json"} << IgnoredConfigJson(1U, dir);

    ASSERT_TRUE(Initialize());
    EXPECT_TRUE(Get(InstanceId{1U}));

    filesystem_test::RemoveAll(dir);
}

TEST_F(kvs_runtime, initialize_from_default_path_missing_is_read_error)
{
    filesystem_test::RemoveAll("./etc/");

    const auto failed{Initialize()};
    ASSERT_FALSE(failed);
    EXPECT_EQ(failed.error(), ErrorCode::kConfigFileReadError);
}

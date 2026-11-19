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
#include "score/kvs/internal/config_json_parsing_strategy.h"

#include "score/json/json_parser.h"
#include "score/kvs/error.h"
#include "score/kvs/kvs_configuration.h"

#include <gtest/gtest.h>

#include <sys/stat.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

using namespace score::kvs;

namespace
{

const std::string kTestDir{"kvs_config_parser_test_dir"};

std::string Doc(const std::string& list)
{
    return R"({"formatVersion": 1, "kvsInstances": )" + list + "}";
}

score::Result<KvsConfiguration> Parse(const std::string& json)
{
    const auto root{score::json::JsonParser{}.FromBuffer(json)};
    EXPECT_TRUE(root.has_value());
    if (!root.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kJsonParserError);
    }
    return ConfigJsonParsingStrategy{}.Parse(root.value());
}

template <typename T>
void ExpectError(const score::Result<T>& result, const ErrorCode expected)
{
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), expected);
}

std::string ReadFile(const std::string& path)
{
    std::ifstream stream{path};
    return std::string{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

score::json::Any ParseFile(const std::string& path)
{
    auto root{score::json::JsonParser{}.FromBuffer(ReadFile(path))};
    EXPECT_TRUE(root.has_value());
    return std::move(root).value();
}

const score::json::Object& ObjectOf(const score::json::Object& parent, const std::string_view key)
{
    const auto it{parent.find(key)};
    EXPECT_NE(it, parent.end());
    return it->second.As<score::json::Object>().value().get();
}

std::set<std::string> KeysOf(const score::json::Object& object)
{
    std::set<std::string> keys{};
    for (const auto& member : object)
    {
        keys.emplace(member.first.GetAsStringView());
    }
    return keys;
}

template <typename Container>
std::set<std::string> SetOf(const Container& values)
{
    return std::set<std::string>{values.begin(), values.end()};
}

std::set<std::string> EnumOf(const score::json::Object& property)
{
    std::set<std::string> values{};
    const auto it{property.find("enum")};
    EXPECT_NE(it, property.end());
    for (const auto& value : it->second.As<score::json::List>().value().get())
    {
        values.emplace(value.As<std::string>().value().get());
    }
    return values;
}

}  // namespace

TEST(kvs_config_parser, full_entry_sets_every_field)
{
    const auto result{Parse(Doc(R"([
        {"instanceId": 2, "snapshotId": 1, "defaults": "required", "kvsLoad": "optional",
         "workingDir": "./data", "maxStorageBytes": 4096, "snapshotMaxCount": 0}
    ])"))};
    ASSERT_TRUE(result.has_value());

    const auto options{result.value().Find(InstanceId{2U})};
    ASSERT_TRUE(options.has_value());
    EXPECT_EQ(options->instance_id, 2U);
    EXPECT_EQ(options->snapshot_id, 1U);
    EXPECT_EQ(options->default_load_policy, KvsDefaults::kRequired);
    EXPECT_EQ(options->current_load_policy, KvsLoad::kOptional);
    EXPECT_EQ(options->working_dir, "./data");
    EXPECT_EQ(options->storage_quota, std::optional<std::size_t>{4096U});
    EXPECT_EQ(options->snapshot_count_limit, 0U);
}

TEST(kvs_config_parser, minimal_entry_takes_builtin_defaults)
{
    const auto result{Parse(Doc(R"([{"instanceId": 0}])"))};
    ASSERT_TRUE(result.has_value());

    const auto options{result.value().Find(InstanceId{0U})};
    ASSERT_TRUE(options.has_value());
    const KvsParameters defaults{InstanceId{0U}};
    EXPECT_EQ(options->snapshot_id, defaults.snapshot_id);
    EXPECT_EQ(options->default_load_policy, defaults.default_load_policy);
    EXPECT_EQ(options->current_load_policy, defaults.current_load_policy);
    EXPECT_EQ(options->working_dir, defaults.working_dir);
    EXPECT_FALSE(options->storage_quota.has_value());
    EXPECT_EQ(options->snapshot_count_limit, defaults.snapshot_count_limit);
}

TEST(kvs_config_parser, empty_list_is_valid)
{
    const auto result{Parse(Doc("[]"))};
    ASSERT_TRUE(result.has_value());
    for (std::size_t id{0U}; id < kMaxInstanceCount; ++id)
    {
        EXPECT_FALSE(result.value().Find(InstanceId{id}).has_value());
    }
}

TEST(kvs_config_parser, boundary_values_are_accepted)
{
    const auto result{Parse(Doc(R"([
        {"instanceId": 0, "snapshotMaxCount": 3, "maxStorageBytes": 1},
        {"instanceId": 9, "snapshotMaxCount": 0}
    ])"))};
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().Find(InstanceId{0U})->snapshot_count_limit, 3U);
    EXPECT_EQ(result.value().Find(InstanceId{0U})->storage_quota, std::optional<std::size_t>{1U});
    EXPECT_EQ(result.value().Find(InstanceId{9U})->snapshot_count_limit, 0U);
}

TEST(kvs_config_parser, load_policy_values_map_to_enums)
{
    const auto result{Parse(Doc(R"([
        {"instanceId": 0, "defaults": "required", "kvsLoad": "optional"},
        {"instanceId": 1, "defaults": "optional", "kvsLoad": "ignored"},
        {"instanceId": 2, "defaults": "ignored", "kvsLoad": "required"}
    ])"))};
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().Find(InstanceId{0U})->default_load_policy, KvsDefaults::kRequired);
    EXPECT_EQ(result.value().Find(InstanceId{0U})->current_load_policy, KvsLoad::kOptional);
    EXPECT_EQ(result.value().Find(InstanceId{1U})->default_load_policy, KvsDefaults::kOptional);
    EXPECT_EQ(result.value().Find(InstanceId{1U})->current_load_policy, KvsLoad::kIgnored);
    EXPECT_EQ(result.value().Find(InstanceId{2U})->default_load_policy, KvsDefaults::kIgnored);
    EXPECT_EQ(result.value().Find(InstanceId{2U})->current_load_policy, KvsLoad::kRequired);
}

struct InvalidCase
{
    std::string name;
    std::string json;
};

class kvs_config_parser_invalid : public ::testing::TestWithParam<InvalidCase>
{
};

TEST_P(kvs_config_parser_invalid, is_rejected)
{
    ExpectError(Parse(GetParam().json), ErrorCode::kInvalidConfiguration);
}

INSTANTIATE_TEST_SUITE_P(
    documents,
    kvs_config_parser_invalid,
    ::testing::Values(
        InvalidCase{"root_is_array", "[]"},
        InvalidCase{"root_is_number", "5"},
        InvalidCase{"format_version_missing", R"({"kvsInstances": []})"},
        InvalidCase{"format_version_two", R"({"formatVersion": 2, "kvsInstances": []})"},
        InvalidCase{"format_version_zero", R"({"formatVersion": 0, "kvsInstances": []})"},
        InvalidCase{"format_version_string", R"({"formatVersion": "1", "kvsInstances": []})"},
        InvalidCase{"list_missing", R"({"formatVersion": 1})"},
        InvalidCase{"list_is_object", R"({"formatVersion": 1, "kvsInstances": {}})"},
        InvalidCase{"unknown_root_key", R"({"formatVersion": 1, "kvsInstances": [], "extra": true})"},
        InvalidCase{"snake_case_root_key", R"({"format_version": 1, "kvsInstances": []})"},
        InvalidCase{"entry_is_number", Doc("[5]")},
        InvalidCase{"entry_is_array", Doc("[[]]")},
        InvalidCase{"instance_id_missing", Doc(R"([{"snapshotId": 1}])")},
        InvalidCase{"instance_id_negative", Doc(R"([{"instanceId": -1}])")},
        InvalidCase{"instance_id_string", Doc(R"([{"instanceId": "1"}])")},
        InvalidCase{"instance_id_fractional", Doc(R"([{"instanceId": 1.5}])")},
        InvalidCase{"instance_id_ten", Doc(R"([{"instanceId": 10}])")},
        InvalidCase{"unknown_entry_key", Doc(R"([{"instanceId": 1, "snapshotMaxCnt": 2}])")},
        InvalidCase{"snake_case_entry_key", Doc(R"([{"instanceId": 1, "working_dir": "./a"}])")},
        InvalidCase{"old_storage_path", Doc(R"([{"instanceId": 1, "storage_path": "./a"}])")},
        InvalidCase{"old_need_defaults", Doc(R"([{"instanceId": 1, "need_defaults": "required"}])")},
        InvalidCase{"old_need_kvs", Doc(R"([{"instanceId": 1, "need_kvs": "required"}])")},
        InvalidCase{"old_max_storage_size_bytes", Doc(R"([{"instanceId": 1, "max_storage_size_bytes": 10}])")},
        InvalidCase{"access_mode", Doc(R"([{"instanceId": 1, "access_mode": "read_only"}])")},
        InvalidCase{"snapshot_id_negative", Doc(R"([{"instanceId": 1, "snapshotId": -1}])")},
        InvalidCase{"snapshot_id_string", Doc(R"([{"instanceId": 1, "snapshotId": "1"}])")},
        InvalidCase{"working_dir_not_string", Doc(R"([{"instanceId": 1, "workingDir": 7}])")},
        InvalidCase{"defaults_unknown_value", Doc(R"([{"instanceId": 1, "defaults": "without"}])")},
        InvalidCase{"defaults_not_string", Doc(R"([{"instanceId": 1, "defaults": true}])")},
        InvalidCase{"kvs_load_unknown_value", Doc(R"([{"instanceId": 1, "kvsLoad": "maybe"}])")},
        InvalidCase{"kvs_load_not_string", Doc(R"([{"instanceId": 1, "kvsLoad": 1}])")},
        InvalidCase{"snapshot_max_count_four", Doc(R"([{"instanceId": 1, "snapshotMaxCount": 4}])")},
        InvalidCase{"snapshot_max_count_negative", Doc(R"([{"instanceId": 1, "snapshotMaxCount": -1}])")},
        InvalidCase{"snapshot_max_count_string", Doc(R"([{"instanceId": 1, "snapshotMaxCount": "2"}])")},
        InvalidCase{"max_storage_bytes_zero", Doc(R"([{"instanceId": 1, "maxStorageBytes": 0}])")},
        InvalidCase{"max_storage_bytes_negative", Doc(R"([{"instanceId": 1, "maxStorageBytes": -1}])")},
        InvalidCase{"max_storage_bytes_fractional", Doc(R"([{"instanceId": 1, "maxStorageBytes": 1.5}])")},
        InvalidCase{"max_storage_bytes_string", Doc(R"([{"instanceId": 1, "maxStorageBytes": "10"}])")},
        InvalidCase{"duplicate_instance_id", Doc(R"([{"instanceId": 3}, {"instanceId": 3}])")},
        InvalidCase{"later_entry_invalid", Doc(R"([{"instanceId": 1}, {"instanceId": 2, "snapshotMaxCount": 9}])")}),
    [](const ::testing::TestParamInfo<InvalidCase>& info) {
        return info.param.name;
    });

class kvs_config_load : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        mkdir(kTestDir.c_str(), 0777);
    }

    void TearDown() override
    {
        for (const auto& file : files_)
        {
            std::remove(file.c_str());
        }
        rmdir(kTestDir.c_str());
    }

    std::string Write(const std::string& name, const std::string& content)
    {
        const std::string path{kTestDir + "/" + name};
        std::ofstream{path} << content;
        files_.push_back(path);
        return path;
    }

  private:
    std::vector<std::string> files_{};
};

TEST_F(kvs_config_load, valid_file_is_loaded)
{
    const auto path{Write("valid.json", Doc(R"([{"instanceId": 3, "workingDir": "./x"}])"))};
    const auto result{KvsConfiguration::Load(path)};
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result.value().Find(InstanceId{3U}).has_value());
    EXPECT_EQ(result.value().Find(InstanceId{3U})->working_dir, "./x");
}

TEST_F(kvs_config_load, missing_file_is_read_error)
{
    ExpectError(KvsConfiguration::Load(kTestDir + "/absent.json"), ErrorCode::kConfigFileReadError);
}

TEST_F(kvs_config_load, invalid_json_is_invalid_configuration)
{
    ExpectError(KvsConfiguration::Load(Write("broken.json", "{")), ErrorCode::kInvalidConfiguration);
}

TEST_F(kvs_config_load, invalid_content_is_invalid_configuration)
{
    ExpectError(KvsConfiguration::Load(Write("content.json", R"({"formatVersion": 2})")),
                ErrorCode::kInvalidConfiguration);
}

TEST_F(kvs_config_load, default_path_missing_is_read_error)
{
    ExpectError(KvsConfiguration::Load(), ErrorCode::kConfigFileReadError);
}

TEST_F(kvs_config_load, extra_instance_can_be_added_after_load)
{
    const auto path{Write("extra.json", Doc(R"([{"instanceId": 1}])"))};
    auto result{KvsConfiguration::Load(path)};
    ASSERT_TRUE(result.has_value());

    EXPECT_TRUE(result.value().Add(KvsParameters{InstanceId{2U}}).has_value());
    EXPECT_TRUE(result.value().Find(InstanceId{1U}).has_value());
    EXPECT_TRUE(result.value().Find(InstanceId{2U}).has_value());
}

TEST_F(kvs_config_load, instance_defined_by_file_cannot_be_added)
{
    const auto path{Write("taken.json", Doc(R"([{"instanceId": 1}])"))};
    auto result{KvsConfiguration::Load(path)};
    ASSERT_TRUE(result.has_value());

    ExpectError(result.value().Add(KvsParameters{InstanceId{1U}}), ErrorCode::kInstanceAlreadyConfigured);
}

TEST(kvs_config_example, example_file_is_loaded)
{
    const auto result{KvsConfiguration::Load("score/kvs/config/kvs_config.example.json")};
    ASSERT_TRUE(result.has_value());

    const auto first{result.value().Find(InstanceId{0U})};
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->snapshot_id, 1U);
    EXPECT_EQ(first->default_load_policy, KvsDefaults::kRequired);
    EXPECT_EQ(first->current_load_policy, KvsLoad::kIgnored);
    EXPECT_EQ(first->working_dir, "./data/settings/");
    EXPECT_EQ(first->storage_quota, std::optional<std::size_t>{1048576U});
    EXPECT_EQ(first->snapshot_count_limit, 2U);

    const auto second{result.value().Find(InstanceId{1U})};
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->working_dir, "./data/cache/");
    EXPECT_EQ(second->snapshot_count_limit, 0U);
    EXPECT_FALSE(second->storage_quota.has_value());

    const auto third{result.value().Find(InstanceId{4U})};
    ASSERT_TRUE(third.has_value());
    EXPECT_EQ(third->working_dir, kDefaultWorkingDir);

    EXPECT_FALSE(result.value().Find(InstanceId{2U}).has_value());
}

TEST(kvs_config_schema, schema_matches_parser_constants)
{
    const auto root{ParseFile("score/kvs/config/kvs_config.schema.json")};
    const auto& schema{root.As<score::json::Object>().value().get()};

    const auto& root_properties{ObjectOf(schema, "properties")};
    EXPECT_EQ(KeysOf(root_properties), SetOf(kConfigRootKeys));

    const auto& version{ObjectOf(root_properties, "formatVersion")};
    EXPECT_EQ(version.find("const")->second.As<std::int64_t>().value(), kConfigFormatVersion);

    const auto& list{ObjectOf(root_properties, "kvsInstances")};
    const auto& entry_properties{ObjectOf(ObjectOf(list, "items"), "properties")};
    EXPECT_EQ(KeysOf(entry_properties), SetOf(kConfigEntryKeys));

    EXPECT_EQ(EnumOf(ObjectOf(entry_properties, "defaults")), SetOf(kConfigLoadPolicyValues));
    EXPECT_EQ(EnumOf(ObjectOf(entry_properties, "kvsLoad")), SetOf(kConfigLoadPolicyValues));

    // The upper bounds in the schema follow the C++ constants.
    const auto maximum{[&entry_properties](const std::string_view key) {
        return ObjectOf(entry_properties, key).find("maximum")->second.As<std::int64_t>().value();
    }};
    EXPECT_EQ(maximum("instanceId"), static_cast<std::int64_t>(kMaxInstanceCount - 1U));
    EXPECT_EQ(maximum("snapshotMaxCount"), static_cast<std::int64_t>(kMaxSnapshotsUpperBound));
}

TEST_F(kvs_config_load, directory_is_read_error)
{
    // A directory opens as a stream but fails on the first read.
    ExpectError(KvsConfiguration::Load(kTestDir), ErrorCode::kConfigFileReadError);
}

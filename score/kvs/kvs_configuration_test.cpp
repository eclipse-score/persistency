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
#include "score/kvs/kvs_configuration.h"

#include <gtest/gtest.h>

using namespace score::kvs;

namespace
{
KvsParameters Options(const std::size_t instance_id)
{
    return KvsParameters{InstanceId{instance_id}};
}
}  // namespace

TEST(kvs_configuration, constructed_options_take_id_and_builtin_defaults)
{
    const KvsParameters options{InstanceId{7U}};
    EXPECT_EQ(options.instance_id, 7U);
    EXPECT_EQ(options.snapshot_id, 0U);
    EXPECT_EQ(options.current_load_policy, KvsLoad::kOptional);
    EXPECT_EQ(options.default_load_policy, KvsDefaults::kOptional);
    EXPECT_EQ(options.working_dir, kDefaultWorkingDir);
    EXPECT_FALSE(options.storage_quota.has_value());
    EXPECT_EQ(options.snapshot_count_limit, kDefaultMaxSnapshots);
}

TEST(kvs_configuration, empty_configuration_finds_nothing)
{
    const KvsConfiguration configuration{};
    for (std::size_t id{0U}; id < kMaxInstanceCount; ++id)
    {
        EXPECT_FALSE(configuration.Find(InstanceId{id}).has_value());
    }
}

TEST(kvs_configuration, add_then_find_returns_options)
{
    KvsParameters options{Options(3U)};
    options.snapshot_id = 1U;
    options.current_load_policy = KvsLoad::kRequired;
    options.default_load_policy = KvsDefaults::kIgnored;
    options.working_dir = "./configuration/";
    options.storage_quota = 1024U;
    options.snapshot_count_limit = 0U;

    KvsConfiguration configuration{};
    ASSERT_TRUE(configuration.Add(options).has_value());

    const auto found{configuration.Find(InstanceId{3U})};
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value().instance_id, 3U);
    EXPECT_EQ(found.value().snapshot_id, 1U);
    EXPECT_EQ(found.value().current_load_policy, KvsLoad::kRequired);
    EXPECT_EQ(found.value().default_load_policy, KvsDefaults::kIgnored);
    EXPECT_EQ(found.value().working_dir, "./configuration/");
    EXPECT_EQ(found.value().storage_quota, std::optional<std::size_t>{1024U});
    EXPECT_EQ(found.value().snapshot_count_limit, 0U);

    EXPECT_FALSE(configuration.Find(InstanceId{2U}).has_value());
}

TEST(kvs_configuration, find_out_of_range_id_returns_nothing)
{
    const KvsConfiguration configuration{};
    EXPECT_FALSE(configuration.Find(InstanceId{kMaxInstanceCount}).has_value());
}

TEST(kvs_configuration, add_accepts_boundary_values)
{
    KvsParameters options{Options(kMaxInstanceCount - 1U)};
    options.snapshot_count_limit = kMaxSnapshotsUpperBound;
    options.storage_quota = 1U;

    KvsConfiguration configuration{};
    EXPECT_TRUE(configuration.Add(options).has_value());
}

TEST(kvs_configuration, add_rejects_out_of_range_instance_id)
{
    KvsConfiguration configuration{};
    const auto result{configuration.Add(Options(kMaxInstanceCount))};
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kInvalidConfiguration);
}

TEST(kvs_configuration, add_rejects_snapshot_count_limit_above_upper_bound)
{
    KvsParameters options{Options(0U)};
    options.snapshot_count_limit = kMaxSnapshotsUpperBound + 1U;

    KvsConfiguration configuration{};
    const auto result{configuration.Add(options)};
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kInvalidConfiguration);
    EXPECT_FALSE(configuration.Find(InstanceId{0U}).has_value());
}

TEST(kvs_configuration, add_rejects_zero_storage_quota)
{
    KvsParameters options{Options(0U)};
    options.storage_quota = 0U;

    KvsConfiguration configuration{};
    const auto result{configuration.Add(options)};
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kInvalidConfiguration);
}

TEST(kvs_configuration, add_rejects_already_configured_instance)
{
    KvsConfiguration configuration{};
    ASSERT_TRUE(configuration.Add(Options(4U)).has_value());

    KvsParameters other{Options(4U)};
    other.working_dir = "./other/";
    const auto result{configuration.Add(other)};
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kInstanceAlreadyConfigured);

    // The first definition stays unchanged.
    const auto found{configuration.Find(InstanceId{4U})};
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value().working_dir, kDefaultWorkingDir);
}

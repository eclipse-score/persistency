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

using namespace score::mw::per::kvs;

TEST(kvs_kvsbuilder, kvsbuilder_build_ignored)
{
    KvsBuilder builder{kInstanceId};
    builder.Snapshot(SnapshotId{0ul});
    builder.Defaults(KvsDefaults::kIgnored);
    builder.KvsLoad(KvsLoad::kIgnored);
    builder.WorkingDir("./kvsbuilder/");
    EXPECT_EQ(builder.instance_id_.id, kInstanceId.id);
    EXPECT_EQ(builder.snapshot_id_.id, 0ul);
    EXPECT_EQ(builder.defaults_, KvsDefaults::kIgnored);
    EXPECT_EQ(builder.kvs_load_, KvsLoad::kIgnored);
    EXPECT_EQ(builder.working_dir_, "./kvsbuilder/");

    auto result_build{builder.Build()};
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvsbuilder/kvs_" + std::to_string(kInstanceId.id));
}

TEST(kvs_kvsbuilder, kvsbuilder_build)
{
    // This test also checks the kvs Open function with the KvsBuilder.

    // Test the KvsBuilder constructor.
    KvsBuilder builder{kInstanceId};
    EXPECT_EQ(builder.instance_id_.id, kInstanceId.id);
    EXPECT_EQ(builder.snapshot_id_.id, 0ul);
    EXPECT_EQ(builder.defaults_, KvsDefaults::kOptional);
    EXPECT_EQ(builder.kvs_load_, KvsLoad::kOptional);

    // Test the KvsBuilder methods.
    builder.Snapshot(SnapshotId{23ul});
    EXPECT_EQ(builder.snapshot_id_.id, 23ul);
    builder.Defaults(KvsDefaults::kRequired);
    EXPECT_EQ(builder.defaults_, KvsDefaults::kRequired);
    builder.KvsLoad(KvsLoad::kRequired);
    EXPECT_EQ(builder.kvs_load_, KvsLoad::kRequired);
    builder.WorkingDir("./kvsbuilder/");
    EXPECT_EQ(builder.working_dir_, "./kvsbuilder/");

    // Test the KvsBuilder Build method.
    // Check that KvsDefaults::kRequired and KvsLoad::kRequired are passed correctly.
    // Build() should return an error, since the files are not available.
    auto result_build{builder.Build()};
    ASSERT_FALSE(result_build);
    // This error occurs in OpenJson() and is passed through Open().
    EXPECT_EQ(static_cast<ErrorCode>(*result_build.error()), ErrorCode::kKvsFileReadError);
    builder.Defaults(KvsDefaults::kOptional);
    result_build = builder.Build();
    ASSERT_FALSE(result_build);
    // This error occurs in OpenJson() and is passed through Open().
    EXPECT_EQ(static_cast<ErrorCode>(*result_build.error()), ErrorCode::kKvsFileReadError);
    builder.KvsLoad(KvsLoad::kOptional);
    result_build = builder.Build();
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvsbuilder/kvs_" + std::to_string(kInstanceId.id));
}

TEST(kvs_kvsbuilder, kvsbuilder_directory_check)
{
    // Test the KvsBuilder with all configurations for the current working directory.
    KvsBuilder builder{kInstanceId};
    builder.WorkingDir("");
    auto result_build{builder.Build()};
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvs_" + std::to_string(kInstanceId.id));

    builder.WorkingDir("./");
    result_build = builder.Build();
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvs_" + std::to_string(kInstanceId.id));

    builder.WorkingDir(".");
    result_build = builder.Build();
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvs_" + std::to_string(kInstanceId.id));
}

TEST(kvs_kvsbuilder, kvsbuilder_snapshot_max_count_default)
{
    // comp_req__kvs__snapshot_max_num_cfg: an unconfigured builder must keep the compile-time default.
    // Existing users then observe no behavior change.
    KvsBuilder builder{kInstanceId};
    EXPECT_EQ(builder.max_snapshots_, kDefaultMaxSnapshots);

    builder.WorkingDir(std::string(kDataDir));
    auto result_build{builder.Build()};
    ASSERT_TRUE(result_build);
    EXPECT_EQ(result_build.value().SnapshotMaxCount(), kDefaultMaxSnapshots);

    CleanupEnvironment();
}

TEST(kvs_kvsbuilder, kvsbuilder_snapshot_max_count_configured)
{
    // comp_req__kvs__snapshot_max_num_cfg: the configured value must reach the built KVS instance.
    // Without the plumbing through Build()/Open() the setter would be silently ignored.
    // Inside the allowed range, and not the default.
    const std::size_t configured_max{2U};

    KvsBuilder builder{kInstanceId};
    builder.SnapshotMaxCount(configured_max);
    EXPECT_EQ(builder.max_snapshots_, configured_max);

    // The setter must be chainable like every other builder option.
    EXPECT_EQ(&builder.SnapshotMaxCount(configured_max), &builder);

    builder.WorkingDir(std::string(kDataDir));
    auto result_build{builder.Build()};
    ASSERT_TRUE(result_build);
    EXPECT_EQ(result_build.value().SnapshotMaxCount(), configured_max);

    CleanupEnvironment();
}

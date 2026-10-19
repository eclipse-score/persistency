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

#include "kvs_general_test.hpp"

TEST(kvs_kvsbuilder, kvsbuilder_build_ignored)
{
    KvsBuilder builder(instance_id);
    builder.snapshot(SnapshotId{0ul});
    builder.need_defaults_flag(OpenNeedDefaults::Ignored);
    builder.need_kvs_flag(OpenNeedKvs::Ignored);
    builder.dir("./kvsbuilder/");
    EXPECT_EQ(builder.instance_id_.id, instance_id.id);
    EXPECT_EQ(builder.snapshot_id_.id, 0ul);
    EXPECT_EQ(builder.need_defaults_, OpenNeedDefaults::Ignored);
    EXPECT_EQ(builder.need_kvs_, OpenNeedKvs::Ignored);
    EXPECT_EQ(builder.directory_, "./kvsbuilder/");

    auto result_build = builder.build();
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvsbuilder/kvs_" + std::to_string(instance_id.id));
}

TEST(kvs_kvsbuilder, kvsbuilder_build)
{
    /* This test also checks the kvs open function with the KvsBuilder */

    /* Test the KvsBuilder constructor */
    KvsBuilder builder(instance_id);
    EXPECT_EQ(builder.instance_id_.id, instance_id.id);
    EXPECT_EQ(builder.snapshot_id_.id, 0ul);
    EXPECT_EQ(builder.need_defaults_, OpenNeedDefaults::Optional);
    EXPECT_EQ(builder.need_kvs_, OpenNeedKvs::Optional);

    /* Test the KvsBuilder methods */
    builder.snapshot(SnapshotId{23ul});
    EXPECT_EQ(builder.snapshot_id_.id, 23ul);
    builder.need_defaults_flag(OpenNeedDefaults::Required);
    EXPECT_EQ(builder.need_defaults_, OpenNeedDefaults::Required);
    builder.need_kvs_flag(OpenNeedKvs::Required);
    EXPECT_EQ(builder.need_kvs_, OpenNeedKvs::Required);
    builder.dir("./kvsbuilder/");
    EXPECT_EQ(builder.directory_, "./kvsbuilder/");

    /* Test the KvsBuilder build method */
    /* We want to check, if OpenNeedDefaults::Required and OpenNeedKvs::Required is passed correctly
    open() function should return an error, since the files are not available */
    auto result_build = builder.build();
    ASSERT_FALSE(result_build);
    EXPECT_EQ(static_cast<ErrorCode>(*result_build.error()),
              ErrorCode::KvsFileReadError); /* This error occurs in open_json and is passed through
                                               open()*/
    builder.need_defaults_flag(OpenNeedDefaults::Optional);
    result_build = builder.build();
    ASSERT_FALSE(result_build);
    EXPECT_EQ(static_cast<ErrorCode>(*result_build.error()),
              ErrorCode::KvsFileReadError); /* This error occurs in open_json and is passed through
                                               open()*/
    builder.need_kvs_flag(OpenNeedKvs::Optional);
    result_build = builder.build();
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvsbuilder/kvs_" + std::to_string(instance_id.id));
}

TEST(kvs_kvsbuilder, kvsbuilder_directory_check)
{
    /* Test the KvsBuilder with all configurations for the current working directory */
    KvsBuilder builder(instance_id);
    builder.dir("");
    auto result_build = builder.build();
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvs_" + std::to_string(instance_id.id));

    builder.dir("./");
    result_build = builder.build();
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvs_" + std::to_string(instance_id.id));

    builder.dir(".");
    result_build = builder.build();
    EXPECT_TRUE(result_build);
    EXPECT_EQ(result_build.value().filename_prefix_.CStr(), "./kvs_" + std::to_string(instance_id.id));
}

TEST(kvs_kvsbuilder, kvsbuilder_snapshot_max_count_default)
{
    /* comp_req__kvs__snapshot_max_num_cfg:
       An unconfigured builder must keep the compile-time default, so that existing
       users observe no behavior change. */
    KvsBuilder builder(instance_id);
    EXPECT_EQ(builder.max_snapshots_, KVS_DEFAULT_MAX_SNAPSHOTS);

    builder.dir(std::string(data_dir));
    auto result_build = builder.build();
    ASSERT_TRUE(result_build);
    EXPECT_EQ(result_build.value().snapshot_max_count(), KVS_DEFAULT_MAX_SNAPSHOTS);

    cleanup_environment();
}

TEST(kvs_kvsbuilder, kvsbuilder_snapshot_max_count_configured)
{
    /* comp_req__kvs__snapshot_max_num_cfg:
       The configured value must reach the built KVS instance. Without the plumbing
       through build()/open() the setter would be silently ignored. */
    const std::size_t configured_max = 2U; /* inside the allowed range, and not the default */

    KvsBuilder builder(instance_id);
    builder.snapshot_max_count(configured_max);
    EXPECT_EQ(builder.max_snapshots_, configured_max);

    /* The setter must be chainable like every other builder option */
    EXPECT_EQ(&builder.snapshot_max_count(configured_max), &builder);

    builder.dir(std::string(data_dir));
    auto result_build = builder.build();
    ASSERT_TRUE(result_build);
    EXPECT_EQ(result_build.value().snapshot_max_count(), configured_max);

    cleanup_environment();
}

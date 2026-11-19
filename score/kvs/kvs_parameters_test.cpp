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

using namespace score::kvs;

TEST(kvs_parameters_builder, defaults)
{
    const KvsParameters params{KvsParametersBuilder{kInstanceId}.Build()};

    EXPECT_EQ(params.instance_id, kInstanceId.id);
    EXPECT_EQ(params.snapshot_id, 0U);
    EXPECT_EQ(params.current_load_policy, KvsLoad::kOptional);
    EXPECT_EQ(params.default_load_policy, KvsDefaults::kOptional);
    EXPECT_EQ(params.working_dir, kDefaultWorkingDir);
    EXPECT_EQ(params.storage_quota, std::nullopt);
    EXPECT_EQ(params.snapshot_count_limit, kDefaultMaxSnapshots);
}

TEST(kvs_parameters_builder, setters)
{
    KvsParametersBuilder builder{kInstanceId};
    builder.Snapshot(SnapshotId{23ul});
    builder.Defaults(KvsDefaults::kRequired);
    builder.KvsLoad(KvsLoad::kIgnored);
    builder.WorkingDir("./kvsbuilder/");
    builder.MaxStorageBytes(std::optional<std::size_t>{1024U});
    builder.SnapshotMaxCount(2U);

    const KvsParameters params{builder.Build()};
    EXPECT_EQ(params.instance_id, kInstanceId.id);
    EXPECT_EQ(params.snapshot_id, 23U);
    EXPECT_EQ(params.default_load_policy, KvsDefaults::kRequired);
    EXPECT_EQ(params.current_load_policy, KvsLoad::kIgnored);
    EXPECT_EQ(params.working_dir, "./kvsbuilder/");
    EXPECT_EQ(params.storage_quota, std::optional<std::size_t>{1024U});
    EXPECT_EQ(params.snapshot_count_limit, 2U);

    builder.MaxStorageBytes(std::nullopt);
    EXPECT_EQ(builder.Build().storage_quota, std::nullopt);
}

TEST(kvs_parameters_builder, setters_are_chainable)
{
    KvsParametersBuilder builder{kInstanceId};

    EXPECT_EQ(&builder.Snapshot(SnapshotId{1ul}), &builder);
    EXPECT_EQ(&builder.Defaults(KvsDefaults::kIgnored), &builder);
    EXPECT_EQ(&builder.KvsLoad(KvsLoad::kIgnored), &builder);
    EXPECT_EQ(&builder.WorkingDir("./kvsbuilder/"), &builder);
    EXPECT_EQ(&builder.MaxStorageBytes(std::nullopt), &builder);
    EXPECT_EQ(&builder.SnapshotMaxCount(1U), &builder);
}

TEST(kvs_parameters_builder, build_is_repeatable)
{
    KvsParametersBuilder builder{kInstanceId};
    builder.WorkingDir("./kvsbuilder/");

    EXPECT_EQ(builder.Build(), builder.Build());
}

TEST(kvs_parameters_builder, snapshot_max_count_clamped)
{
    // A request above the upper bound is stored clamped to kMaxSnapshotsUpperBound.
    EXPECT_EQ(KvsParametersBuilder{kInstanceId}.SnapshotMaxCount(100U).Build().snapshot_count_limit,
              kMaxSnapshotsUpperBound);
    EXPECT_EQ(
        KvsParametersBuilder{kInstanceId}.SnapshotMaxCount(kMaxSnapshotsUpperBound + 1U).Build().snapshot_count_limit,
        kMaxSnapshotsUpperBound);

    // Values inside the range are kept, including the bound itself and 0.
    EXPECT_EQ(KvsParametersBuilder{kInstanceId}.SnapshotMaxCount(kMaxSnapshotsUpperBound).Build().snapshot_count_limit,
              kMaxSnapshotsUpperBound);
    EXPECT_EQ(KvsParametersBuilder{kInstanceId}.SnapshotMaxCount(0U).Build().snapshot_count_limit, 0U);

    // The clamped request equals a request for the bound.
    EXPECT_EQ(KvsParametersBuilder{kInstanceId}.SnapshotMaxCount(100U).Build(),
              KvsParametersBuilder{kInstanceId}.SnapshotMaxCount(kMaxSnapshotsUpperBound).Build());
}

TEST(kvs_parameters_builder, instance_id_range_edges)
{
    // The builder does not validate the ID, the configuration and the runtime do.
    EXPECT_EQ(KvsParametersBuilder{InstanceId{0U}}.Build().instance_id, 0U);
    EXPECT_EQ(KvsParametersBuilder{InstanceId{kMaxInstanceCount - 1U}}.Build().instance_id, kMaxInstanceCount - 1U);
    EXPECT_EQ(KvsParametersBuilder{InstanceId{kMaxInstanceCount}}.Build().instance_id, kMaxInstanceCount);
}

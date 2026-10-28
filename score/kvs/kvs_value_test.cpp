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

#include "score/kvs/kvs_value.h"

#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <variant>

using namespace score::kvs;

TEST(kvs_value_test, get_type_reports_variant)
{
    const KvsValue signed_32(std::int32_t{42});
    const KvsValue unsigned_32(std::uint32_t{42});
    const KvsValue signed_64(std::int64_t{42});
    const KvsValue unsigned_64(std::uint64_t{42});
    const KvsValue floating_point(42.0);
    const KvsValue boolean(true);
    const KvsValue string_value("text");
    const KvsValue null_value(nullptr);
    const KvsValue array_value(KvsValue::Array{});
    const KvsValue object_value(KvsValue::Object{});

    EXPECT_EQ(signed_32.getType(), KvsValue::Type::i32);
    EXPECT_EQ(unsigned_32.getType(), KvsValue::Type::u32);
    EXPECT_EQ(signed_64.getType(), KvsValue::Type::i64);
    EXPECT_EQ(unsigned_64.getType(), KvsValue::Type::u64);
    EXPECT_EQ(floating_point.getType(), KvsValue::Type::f64);
    EXPECT_EQ(boolean.getType(), KvsValue::Type::Boolean);
    EXPECT_EQ(string_value.getType(), KvsValue::Type::String);
    EXPECT_EQ(null_value.getType(), KvsValue::Type::Null);
    EXPECT_EQ(array_value.getType(), KvsValue::Type::Array);
    EXPECT_EQ(object_value.getType(), KvsValue::Type::Object);
}

TEST(kvs_value_test, get_type_follows_assignment)
{
    KvsValue value(std::int32_t{1});
    const KvsValue text("text");

    value = text;
    EXPECT_EQ(value.getType(), KvsValue::Type::String);

    value = KvsValue(KvsValue::Array{});
    EXPECT_EQ(value.getType(), KvsValue::Type::Array);

    KvsValue moved_from(std::int32_t{2});
    value = std::move(moved_from);
    EXPECT_EQ(value.getType(), KvsValue::Type::i32);
}

TEST(kvs_value_test, map_constructor_yields_object)
{
    const std::unordered_map<std::string, KvsValue> map{{"key", KvsValue(true)}};
    const KvsValue value(map);

    ASSERT_EQ(value.getType(), KvsValue::Type::Object);
    const auto& object = std::get<KvsValue::Object>(value.getValue());
    ASSERT_EQ(object.size(), 1U);
    EXPECT_EQ(object.front().first, "key");
}

TEST(kvs_value_test, move_constructor_preserves_type)
{
    KvsValue source(std::string{"text"});
    KvsValue value(std::move(source));

    EXPECT_EQ(value.getType(), KvsValue::Type::String);
}

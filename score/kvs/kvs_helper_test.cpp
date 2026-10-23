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

TEST(kvs_calculate_hash_adler32, calculate_hash_adler32)
{
    // Test the Adler-32 hash calculation.
    // Parsing test is done automatically by the open tests.

    std::string test_data = "Hello, World!";
    uint32_t calculated_hash = adler32(test_data);
    EXPECT_EQ(calculated_hash, calculate_hash_adler32(test_data));

    std::array<uint8_t, 4> buf{};
    std::istringstream stream(test_data);
    stream.read(reinterpret_cast<char*>(buf.data()), buf.size());

    std::array<uint8_t, 4> value = {uint8_t((calculated_hash >> 24) & 0xFF),
                                    uint8_t((calculated_hash >> 16) & 0xFF),
                                    uint8_t((calculated_hash >> 8) & 0xFF),
                                    uint8_t((calculated_hash) & 0xFF)};
    EXPECT_EQ(value, get_hash_bytes(test_data));
}

TEST(kvs_calculate_hash_adler32, calculate_hash_adler32_large_data)
{
    // Create a test string with more than 5552 characters to check that the hash is calculated correctly.
    std::string large_data(6000, 'A');
    uint32_t hash = calculate_hash_adler32(large_data);

    EXPECT_EQ(adler32(large_data), hash);
}

TEST(kvs_check_hash, check_hash_valid)
{
    std::string test_data = "Hello, World!";
    uint32_t hash = adler32(test_data);
    std::array<uint8_t, 4> hash_bytes = {uint8_t((hash >> 24) & 0xFF),
                                         uint8_t((hash >> 16) & 0xFF),
                                         uint8_t((hash >> 8) & 0xFF),
                                         uint8_t((hash) & 0xFF)};
    std::string hash_string(reinterpret_cast<const char*>(hash_bytes.data()), hash_bytes.size());
    std::istringstream hash_stream(hash_string);

    EXPECT_TRUE(check_hash(test_data, hash_stream));
}

TEST(kvs_check_hash, check_hash_invalid)
{
    std::string test_data = "Hello, World!";
    uint32_t hash = adler32(test_data);
    std::array<uint8_t, 4> hash_bytes = {uint8_t((hash >> 24) & 0xFF),
                                         uint8_t((hash >> 16) & 0xFF),
                                         uint8_t((hash >> 8) & 0xFF),
                                         uint8_t((hash) & 0xFF)};
    std::string hash_string(reinterpret_cast<const char*>(hash_bytes.data()), hash_bytes.size());
    std::istringstream hash_stream(hash_string);

    std::string test_data_invalid = "Hello, invalid World!";

    EXPECT_FALSE(check_hash(test_data_invalid, hash_stream));
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_bool)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("bool")));
    obj.emplace("v", score::json::Any(true));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::Boolean);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_i32)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("i32")));
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::i32);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_u32)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("u32")));
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::u32);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_i64)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("i64")));
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::i64);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_u64)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("u64")));
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::u64);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_f64)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("f64")));
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::f64);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_string)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("str")));
    obj.emplace("v", score::json::Any(std::string("test")));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::String);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_null)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("null")));
    obj.emplace("v", score::json::Any(score::json::Null()));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::Null);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_array)
{
    score::json::List list;
    score::json::Object inner_obj_1;
    inner_obj_1.emplace("t", score::json::Any(std::string("bool")));
    inner_obj_1.emplace("v", score::json::Any(true));
    list.push_back(score::json::Any(std::move(inner_obj_1)));

    score::json::Object inner_obj_2;
    inner_obj_2.emplace("t", score::json::Any(std::string("f64")));
    inner_obj_2.emplace("v", score::json::Any(1.1));
    list.push_back(score::json::Any(std::move(inner_obj_2)));

    score::json::Object inner_obj_3;
    inner_obj_3.emplace("t", score::json::Any(std::string("str")));
    inner_obj_3.emplace("v", score::json::Any(std::string("test")));
    list.push_back(score::json::Any(std::move(inner_obj_3)));

    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("arr")));
    obj.emplace("v", score::json::Any(std::move(list)));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::Array);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_object)
{
    score::json::Object inner_obj_1;
    inner_obj_1.emplace("t", score::json::Any(std::string("bool")));
    inner_obj_1.emplace("v", score::json::Any(true));

    score::json::Object inner_obj_2;
    inner_obj_2.emplace("t", score::json::Any(std::string("f64")));
    inner_obj_2.emplace("v", score::json::Any(42.0));

    score::json::Object combined_obj;
    combined_obj.emplace("flag", score::json::Any(std::move(inner_obj_1)));
    combined_obj.emplace("count", score::json::Any(std::move(inner_obj_2)));

    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("obj")));
    obj.emplace("v", score::json::Any(std::move(combined_obj)));

    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().getType(), KvsValue::Type::Object);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_object_over_length_key_accepted)
{
    // comp_req__kvs__key_length applies to root keys only.
    // Keys of nested objects are value data.
    const std::string long_key(KVS_MAX_KEY_LENGTH_BYTES + 1U, 'x');

    score::json::Object inner_obj;
    inner_obj.emplace("t", score::json::Any(std::string("i32")));
    inner_obj.emplace("v", score::json::Any(1));

    score::json::Object combined_obj;
    combined_obj.emplace(long_key, score::json::Any(std::move(inner_obj)));

    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("obj")));
    obj.emplace("v", score::json::Any(std::move(combined_obj)));

    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    ASSERT_TRUE(result);
    ASSERT_EQ(result.value().getType(), KvsValue::Type::Object);
    const auto& result_obj = std::get<KvsValue::Object>(result.value().getValue());
    ASSERT_EQ(result_obj.size(), 1U);
    EXPECT_EQ(result_obj[0].first, long_key);
    ASSERT_EQ(result_obj[0].second.getType(), KvsValue::Type::i32);
    EXPECT_EQ(std::get<int32_t>(result_obj[0].second.getValue()), 1);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_format_invalid)
{
    score::json::Object obj_type;
    obj_type.emplace("invalid", score::json::Any(std::string("bool")));
    obj_type.emplace("v", score::json::Any(true));
    score::json::Any any_obj_type(std::move(obj_type));
    auto result = any_to_kvsvalue(any_obj_type);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);

    score::json::Object obj_value;
    obj_value.emplace("t", score::json::Any(std::string("bool")));
    obj_value.emplace("invalid", score::json::Any(true));
    score::json::Any any_obj_value(std::move(obj_value));
    result = any_to_kvsvalue(any_obj_value);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_no_object)
{
    score::json::Any any_bool(true);
    auto result = any_to_kvsvalue(any_bool);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_type_no_string)
{
    score::json::Object obj;
    // Not a string.
    obj.emplace("t", score::json::Any(42.0));
    obj.emplace("v", score::json::Any(true));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_type_invalid)
{
    score::json::Object obj;
    obj.emplace("t", "invalid");
    obj.emplace("v", score::json::Any(true));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_no_string_type)
{
    score::json::Object obj;
    // Not a string.
    obj.emplace("t", score::json::Any(42.0));
    obj.emplace("v", score::json::Any(true));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_i32)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("i32")));
    // Invalid value for I32.
    obj.emplace("v", score::json::Any("invalid"));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_u32)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("u32")));
    // Invalid value for U32.
    obj.emplace("v", score::json::Any("invalid"));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_i64)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("i64")));
    // Invalid value for I64.
    obj.emplace("v", score::json::Any("invalid"));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_u64)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("u64")));
    // Invalid value for U64.
    obj.emplace("v", score::json::Any("invalid"));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_f64)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("f64")));
    // Invalid value for F64.
    obj.emplace("v", score::json::Any("invalid"));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_boolean)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("bool")));
    // Invalid value for Boolean.
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_string)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("str")));
    // Invalid value for String.
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_null)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("null")));
    // Invalid value for Null.
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_array)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("arr")));
    // Invalid value for Array.
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_invalid_object)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("obj")));
    // Invalid value for Object.
    obj.emplace("v", score::json::Any(42.0));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_array_with_invalid_element)
{
    score::json::List list;

    score::json::Object inner_obj1;
    inner_obj1.emplace("t", score::json::Any(std::string("bool")));
    inner_obj1.emplace("v", score::json::Any(true));
    list.push_back(score::json::Any(std::move(inner_obj1)));

    score::json::Object inner_obj2;
    inner_obj2.emplace("t", score::json::Any(std::string("InvalidType")));
    inner_obj2.emplace("v", score::json::Any(std::string("test")));
    list.push_back(score::json::Any(std::move(inner_obj2)));

    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("arr")));
    obj.emplace("v", score::json::Any(std::move(list)));
    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_object_with_invalid_value)
{
    score::json::Object inner_obj1;
    inner_obj1.emplace("t", score::json::Any(std::string("bool")));
    inner_obj1.emplace("v", score::json::Any(true));

    score::json::Object inner_obj2;
    inner_obj2.emplace("t", score::json::Any(std::string("InvalidType")));
    inner_obj2.emplace("v", score::json::Any(42.0));

    score::json::Object value_obj;
    value_obj.emplace("flag", score::json::Any(std::move(inner_obj1)));
    value_obj.emplace("count", score::json::Any(std::move(inner_obj2)));

    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("obj")));
    obj.emplace("v", score::json::Any(std::move(value_obj)));

    score::json::Any any_obj(std::move(obj));
    auto result = any_to_kvsvalue(any_obj);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_null)
{
    KvsValue null_val(nullptr);
    auto result = kvsvalue_to_any(null_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "null");
    EXPECT_TRUE(obj.at("v").As<score::json::Null>().has_value());
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_boolean)
{
    KvsValue bool_val(true);
    auto result = kvsvalue_to_any(bool_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "bool");
    EXPECT_EQ(obj.at("v").As<bool>().value(), true);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_i32)
{
    KvsValue i32_val(static_cast<int32_t>(42));
    auto result = kvsvalue_to_any(i32_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "i32");
    EXPECT_EQ(obj.at("v").As<int32_t>().value(), 42.0);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_u32)
{
    KvsValue u32_val(static_cast<uint32_t>(42));
    auto result = kvsvalue_to_any(u32_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "u32");
    EXPECT_EQ(obj.at("v").As<uint32_t>().value(), 42.0);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_i64)
{
    KvsValue i64_val(static_cast<int64_t>(42));
    auto result = kvsvalue_to_any(i64_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "i64");
    EXPECT_EQ(obj.at("v").As<int64_t>().value(), 42.0);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_u64)
{
    KvsValue u64_val(static_cast<uint64_t>(42));
    auto result = kvsvalue_to_any(u64_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "u64");
    EXPECT_EQ(obj.at("v").As<uint64_t>().value(), 42.0);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_f64)
{
    KvsValue f64_val(42.0);
    auto result = kvsvalue_to_any(f64_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "f64");
    EXPECT_EQ(obj.at("v").As<double>().value(), 42.0);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_string)
{
    std::string str = "test";
    KvsValue string_val(str);
    auto result = kvsvalue_to_any(string_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "str");
    EXPECT_EQ(obj.at("v").As<std::string>().value().get(), "test");
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_array)
{
    KvsValue::Array array;
    array.push_back(KvsValue(true));
    array.push_back(KvsValue(1.1));
    array.push_back(KvsValue(std::string("test")));
    KvsValue array_val(array);
    auto result = kvsvalue_to_any(array_val);
    ASSERT_TRUE(result);
    const auto& obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "arr");
    const auto& list = obj.at("v").As<score::json::List>().value().get();
    EXPECT_EQ(list.size(), 3);

    const auto& elem0 = list[0].As<score::json::Object>().value().get();
    EXPECT_EQ(elem0.at("t").As<std::string>().value().get(), "bool");
    EXPECT_EQ(elem0.at("v").As<bool>().value(), true);

    const auto& elem1 = list[1].As<score::json::Object>().value().get();
    EXPECT_EQ(elem1.at("t").As<std::string>().value().get(), "f64");
    EXPECT_EQ(elem1.at("v").As<double>().value(), 1.1);

    const auto& elem2 = list[2].As<score::json::Object>().value().get();
    EXPECT_EQ(elem2.at("t").As<std::string>().value().get(), "str");
    EXPECT_EQ(elem2.at("v").As<std::string>().value().get(), "test");
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_object)
{
    KvsValue::Object obj;
    // Boolean.
    obj.emplace_back("flag", KvsValue(true));
    // F64.
    obj.emplace_back("count", KvsValue(42.0));
    KvsValue obj_val(obj);

    auto result = kvsvalue_to_any(obj_val);
    ASSERT_TRUE(result);

    const auto& outer_obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(outer_obj.at("t").As<std::string>().value().get(), "obj");

    const auto& inner_obj = outer_obj.at("v").As<score::json::Object>().value().get();

    const auto& flag_entry = inner_obj.at("flag").As<score::json::Object>().value().get();
    EXPECT_EQ(flag_entry.at("t").As<std::string>().value().get(), "bool");
    EXPECT_EQ(flag_entry.at("v").As<bool>().value(), true);

    const auto& count_entry = inner_obj.at("count").As<score::json::Object>().value().get();
    EXPECT_EQ(count_entry.at("t").As<std::string>().value().get(), "f64");
    EXPECT_EQ(count_entry.at("v").As<double>().value(), 42.0);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_object_duplicate_key)
{
    KvsValue::Object obj{{"key", KvsValue(int32_t{1})}, {"key", KvsValue(int32_t{2})}};
    auto result = kvsvalue_to_any(KvsValue(obj));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);

    // Duplicate key in an object nested in an array.
    KvsValue::Array array{KvsValue(std::move(obj))};
    result = kvsvalue_to_any(KvsValue(std::move(array)));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_map)
{
    // A map is serialized directly, without wrapping it in a KvsValue.
    KvsMap map;
    map.emplace("flag", KvsValue(true));
    map.emplace("name", KvsValue(std::string("test")));
    map.emplace("nested", KvsValue(KvsValue::Object{{"count", KvsValue(int32_t{7})}}));

    auto result = kvsvalue_to_any(map);
    ASSERT_TRUE(result);

    const auto& outer_obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(outer_obj.at("t").As<std::string>().value().get(), "obj");

    const auto& inner_obj = outer_obj.at("v").As<score::json::Object>().value().get();
    EXPECT_EQ(inner_obj.size(), 3U);

    const auto& flag_entry = inner_obj.at("flag").As<score::json::Object>().value().get();
    EXPECT_EQ(flag_entry.at("t").As<std::string>().value().get(), "bool");
    EXPECT_EQ(flag_entry.at("v").As<bool>().value(), true);

    const auto& name_entry = inner_obj.at("name").As<score::json::Object>().value().get();
    EXPECT_EQ(name_entry.at("t").As<std::string>().value().get(), "str");
    EXPECT_EQ(name_entry.at("v").As<std::string>().value().get(), "test");

    const auto& nested_entry = inner_obj.at("nested").As<score::json::Object>().value().get();
    EXPECT_EQ(nested_entry.at("t").As<std::string>().value().get(), "obj");
    const auto& nested_obj = nested_entry.at("v").As<score::json::Object>().value().get();
    const auto& count_entry = nested_obj.at("count").As<score::json::Object>().value().get();
    EXPECT_EQ(count_entry.at("t").As<std::string>().value().get(), "i32");
    EXPECT_EQ(count_entry.at("v").As<int32_t>().value(), 7);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_map_empty)
{
    KvsMap map;
    auto result = kvsvalue_to_any(map);
    ASSERT_TRUE(result);

    const auto& outer_obj = result.value().As<score::json::Object>().value().get();
    EXPECT_EQ(outer_obj.at("t").As<std::string>().value().get(), "obj");
    EXPECT_TRUE(outer_obj.at("v").As<score::json::Object>().value().get().empty());
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_map_invalid)
{
    KvsMap map;
    map.emplace("valid", KvsValue(42.0));
    map.emplace("invalid", KvsValue(KvsValue::Object{{"key", KvsValue(true)}, {"key", KvsValue(false)}}));

    auto result = kvsvalue_to_any(map);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::InvalidValueType);
}

TEST(kvs_has_unique_keys, has_unique_keys_scalar)
{
    EXPECT_TRUE(has_unique_keys(KvsValue(int32_t{1})));
    EXPECT_TRUE(has_unique_keys(KvsValue("text")));
    EXPECT_TRUE(has_unique_keys(KvsValue(nullptr)));
}

TEST(kvs_has_unique_keys, has_unique_keys_unique)
{
    KvsValue::Object inner{{"a", KvsValue(true)}, {"b", KvsValue(false)}};
    KvsValue::Array array{KvsValue(inner), KvsValue(inner)};
    KvsValue::Object outer{{"a", KvsValue(std::move(inner))}, {"b", KvsValue(std::move(array))}};
    EXPECT_TRUE(has_unique_keys(KvsValue(std::move(outer))));
}

TEST(kvs_has_unique_keys, has_unique_keys_duplicate)
{
    KvsValue::Object obj{{"a", KvsValue(true)}, {"a", KvsValue(false)}};
    EXPECT_FALSE(has_unique_keys(KvsValue(obj)));
}

TEST(kvs_has_unique_keys, has_unique_keys_duplicate_nested_in_object)
{
    KvsValue::Object inner{{"a", KvsValue(true)}, {"a", KvsValue(false)}};
    KvsValue::Object outer{{"inner", KvsValue(std::move(inner))}};
    EXPECT_FALSE(has_unique_keys(KvsValue(std::move(outer))));
}

TEST(kvs_has_unique_keys, has_unique_keys_duplicate_nested_in_array)
{
    KvsValue::Object inner{{"a", KvsValue(true)}, {"a", KvsValue(false)}};
    KvsValue::Array array{KvsValue(int32_t{0}), KvsValue(std::move(inner))};
    EXPECT_FALSE(has_unique_keys(KvsValue(std::move(array))));
}

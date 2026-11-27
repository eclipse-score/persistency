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

#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

using namespace score::kvs;

namespace
{

score::json::Any MakeTaggedValue(std::string tag, score::json::Any value)
{
    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::move(tag)));
    obj.emplace("v", std::move(value));
    return score::json::Any(std::move(obj));
}

void ExpectInvalid(const score::json::Any& any)
{
    const auto result{AnyToKvsValue(any)};
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kInvalidValueType);
}

void ExpectInvalidTaggedValue(std::string tag, score::json::Any value)
{
    SCOPED_TRACE(tag);
    ExpectInvalid(MakeTaggedValue(std::move(tag), std::move(value)));
}

template <typename T>
void ExpectTaggedValue(const KvsValue& value, const std::string_view tag, const T& expected)
{
    SCOPED_TRACE(tag);
    const auto result{KvsValueToAny(value)};
    ASSERT_TRUE(result);
    const auto object{result->As<score::json::Object>()};
    ASSERT_TRUE(object);
    const auto& members{object->get()};
    ASSERT_EQ(members.at("t").As<std::string>().value().get(), tag);
    const auto actual{members.at("v").As<T>()};
    ASSERT_TRUE(actual);
    if constexpr (std::is_same_v<T, std::string>)
    {
        EXPECT_EQ(actual.value().get(), expected);
    }
    else
    {
        EXPECT_EQ(actual.value(), expected);
    }
}

}  // namespace

TEST(kvs_calculate_hash_adler32, calculate_hash_adler32)
{
    // Test the Adler-32 hash calculation.
    // Parsing test is done automatically by the open tests.

    std::string test_data{"Hello, World!"};
    uint32_t calculated_hash{Adler32(test_data)};
    EXPECT_EQ(calculated_hash, CalculateHashAdler32(test_data));

    std::array<uint8_t, 4> buffer{};
    std::istringstream stream{test_data};
    stream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    std::array<uint8_t, 4> value{uint8_t((calculated_hash >> 24) & 0xFF),
                                 uint8_t((calculated_hash >> 16) & 0xFF),
                                 uint8_t((calculated_hash >> 8) & 0xFF),
                                 uint8_t((calculated_hash) & 0xFF)};
    EXPECT_EQ(value, GetHashBytes(test_data));
}

TEST(kvs_calculate_hash_adler32, calculate_hash_adler32_large_data)
{
    // Create a test string with more than 5552 characters to check that the hash is calculated correctly.
    std::string large_data(6000, 'A');
    uint32_t hash{CalculateHashAdler32(large_data)};

    EXPECT_EQ(Adler32(large_data), hash);
}

TEST(kvs_check_hash, check_hash_valid)
{
    std::string test_data{"Hello, World!"};
    uint32_t hash{Adler32(test_data)};
    std::array<uint8_t, 4> hash_bytes{uint8_t((hash >> 24) & 0xFF),
                                      uint8_t((hash >> 16) & 0xFF),
                                      uint8_t((hash >> 8) & 0xFF),
                                      uint8_t((hash) & 0xFF)};
    std::string hash_string{reinterpret_cast<const char*>(hash_bytes.data()), hash_bytes.size()};
    std::istringstream hash_stream{hash_string};

    EXPECT_TRUE(CheckHash(test_data, hash_stream));
}

TEST(kvs_check_hash, check_hash_invalid)
{
    std::string test_data{"Hello, World!"};
    uint32_t hash{Adler32(test_data)};
    std::array<uint8_t, 4> hash_bytes{uint8_t((hash >> 24) & 0xFF),
                                      uint8_t((hash >> 16) & 0xFF),
                                      uint8_t((hash >> 8) & 0xFF),
                                      uint8_t((hash) & 0xFF)};
    std::string hash_string{reinterpret_cast<const char*>(hash_bytes.data()), hash_bytes.size()};
    std::istringstream hash_stream{hash_string};

    std::string test_data_invalid{"Hello, invalid World!"};

    EXPECT_FALSE(CheckHash(test_data_invalid, hash_stream));
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_bool)
{
    auto result{AnyToKvsValue(MakeTaggedValue("bool", score::json::Any(true)))};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().GetType(), KvsValue::Type::kBoolean);
}

TEST(kvs_any_to_kvsvalue, numeric_tags_map_to_types)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_serialize");
    RecordProperty("Description", "Check that every numeric type tag deserializes to the matching KvsValue type.");

    const std::pair<const char*, KvsValue::Type> cases[]{
        {"u8", KvsValue::Type::kU8},
        {"u16", KvsValue::Type::kU16},
        {"u32", KvsValue::Type::kU32},
        {"u64", KvsValue::Type::kU64},
        {"i8", KvsValue::Type::kI8},
        {"i16", KvsValue::Type::kI16},
        {"i32", KvsValue::Type::kI32},
        {"i64", KvsValue::Type::kI64},
        {"f32", KvsValue::Type::kF32},
        {"f64", KvsValue::Type::kF64},
    };
    for (const auto& [tag, type] : cases)
    {
        SCOPED_TRACE(tag);
        const auto result{AnyToKvsValue(MakeTaggedValue(tag, score::json::Any(42.0)))};
        ASSERT_TRUE(result);
        EXPECT_EQ(result->GetType(), type);
    }
}

TEST(kvs_any_to_kvsvalue, numeric_values_preserve_large_values)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_serialize");
    RecordProperty("Description",
                   "Check that tagged f32 and f64 values deserialize when their JSON numbers use wider numeric types.");

    auto f32_result{AnyToKvsValue(MakeTaggedValue("f32", score::json::Any(123456.0)))};
    ASSERT_TRUE(f32_result);
    ASSERT_EQ(f32_result->GetType(), KvsValue::Type::kF32);
    EXPECT_FLOAT_EQ(std::get<float>(f32_result->GetValue()), 123456.0F);

    const uint64_t large_integer{uint64_t{1U} << 40U};
    auto f64_result{AnyToKvsValue(MakeTaggedValue("f64", score::json::Any(large_integer)))};
    ASSERT_TRUE(f64_result);
    ASSERT_EQ(f64_result->GetType(), KvsValue::Type::kF64);
    EXPECT_DOUBLE_EQ(std::get<double>(f64_result->GetValue()), static_cast<double>(large_integer));
}

TEST(kvs_any_to_kvsvalue, numeric_values_reject_invalid_numbers_and_booleans)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_serialize");
    RecordProperty(
        "Description",
        "Check that numeric tags reject booleans, fractional integers, and non-finite or overflowing floats.");

    ExpectInvalidTaggedValue("u8", score::json::Any(true));
    ExpectInvalidTaggedValue("i8", score::json::Any(1.5));
    ExpectInvalidTaggedValue("f32", score::json::Any(std::numeric_limits<double>::infinity()));
    ExpectInvalidTaggedValue("f32", score::json::Any(std::numeric_limits<double>::max()));
}

namespace
{

score::Result<KvsValue> ParseTaggedText(const std::string& tag, const std::string& number)
{
    const score::json::JsonParser parser;
    const std::string text{R"({"t":")" + tag + R"(","v":)" + number + "}"};
    const auto parsed{parser.FromBuffer(text)};
    if (!parsed.has_value())
    {
        ADD_FAILURE() << "Failed to parse: " << text;
        return score::MakeUnexpected(ErrorCode::kJsonParserError);
    }
    return AnyToKvsValue(parsed.value());
}

}  // namespace

TEST(kvs_any_to_kvsvalue, numeric_values_reject_out_of_range)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_data_types, comp_req__kvs__value_serialize");
    RecordProperty("Description",
                   "Check that numeric tags accept parsed JSON text at the range limits of the tagged type and "
                   "reject parsed JSON text one step outside of it.");

    struct RangeCase
    {
        const char* tag;
        const char* in_range;
        const char* out_of_range;
    };
    const RangeCase cases[]{
        {"u8", "0", "-1"},
        {"u8", "255", "256"},
        {"i8", "-128", "-129"},
        {"i8", "127", "128"},
        {"u16", "0", "-1"},
        {"u16", "65535", "65536"},
        {"i16", "-32768", "-32769"},
        {"i16", "32767", "32768"},
        {"u32", "0", "-1"},
        {"u32", "4294967295", "4294967296"},
        {"i32", "-2147483648", "-2147483649"},
        {"i32", "2147483647", "2147483648"},
        {"u64", "0", "-1"},
        {"u64", "18446744073709551615", "18446744073709551616"},
        {"i64", "-9223372036854775808", "-9223372036854775809"},
        {"i64", "9223372036854775807", "9223372036854775808"},
    };
    for (const auto& range_case : cases)
    {
        SCOPED_TRACE(std::string{range_case.tag} + " " + range_case.in_range + " / " + range_case.out_of_range);

        const auto accepted{ParseTaggedText(range_case.tag, range_case.in_range)};
        EXPECT_TRUE(accepted.has_value());

        const auto rejected{ParseTaggedText(range_case.tag, range_case.out_of_range)};
        ASSERT_FALSE(rejected.has_value());
        EXPECT_EQ(rejected.error(), ErrorCode::kInvalidValueType);
    }

    // Smallest double that no longer rounds to the largest finite float.
    ExpectInvalidTaggedValue(
        "f32", score::json::Any(static_cast<double>(std::numeric_limits<float>::max()) + std::ldexp(1.0, 103)));
    ExpectInvalidTaggedValue(
        "f32", score::json::Any(-static_cast<double>(std::numeric_limits<float>::max()) - std::ldexp(1.0, 103)));
}

TEST(kvs_any_to_kvsvalue, floating_point_values_accept_underflow_as_zero)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_data_types, comp_req__kvs__value_serialize");
    RecordProperty("Description",
                   "Check that finite floating point values below the smallest representable magnitude of the "
                   "tagged type are accepted and rounded to zero, matching the Rust implementation.");

    const std::pair<const char*, bool> f32_underflows[]{
        {"1e-50", false}, {"1e-46", false}, {"7e-46", false}, {"-1e-50", true}};
    for (const auto& [text, negative] : f32_underflows)
    {
        SCOPED_TRACE(text);
        const auto result{ParseTaggedText("f32", text)};
        ASSERT_TRUE(result);
        const float value{std::get<float>(result->GetValue())};
        EXPECT_EQ(value, 0.0F);
        EXPECT_EQ(std::signbit(value), negative);
    }

    const auto f64_underflow{ParseTaggedText("f64", "1e-400")};
    ASSERT_TRUE(f64_underflow);
    EXPECT_EQ(std::get<double>(f64_underflow->GetValue()), 0.0);
}

TEST(kvs_any_to_kvsvalue, negative_zero_loses_sign_in_json_text)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_data_types, comp_req__kvs__value_serialize");
    RecordProperty("Description",
                   "Check the documented baselibs limit that negative zero is written as -0 and parsed back as an "
                   "integer, so floating point values load as positive zero and unsigned tags reject it.");

    const auto write_and_parse = [](const KvsValue& value) -> score::Result<KvsValue> {
        const auto any{KvsValueToAny(value)};
        EXPECT_TRUE(any);
        score::json::JsonWriter writer;
        const auto buffer{writer.ToBuffer(any.value())};
        EXPECT_TRUE(buffer);
        EXPECT_NE(buffer->find("-0"), std::string::npos) << buffer.value();
        const score::json::JsonParser parser;
        const auto parsed{parser.FromBuffer(buffer.value())};
        EXPECT_TRUE(parsed) << buffer.value();
        return AnyToKvsValue(parsed.value());
    };

    const auto f32_zero{write_and_parse(KvsValue{-0.0F})};
    ASSERT_TRUE(f32_zero);
    EXPECT_EQ(std::get<float>(f32_zero->GetValue()), 0.0F);
    EXPECT_FALSE(std::signbit(std::get<float>(f32_zero->GetValue())));

    const auto f64_zero{write_and_parse(KvsValue{-0.0})};
    ASSERT_TRUE(f64_zero);
    EXPECT_EQ(std::get<double>(f64_zero->GetValue()), 0.0);
    EXPECT_FALSE(std::signbit(std::get<double>(f64_zero->GetValue())));

    // `-0.0` is not an integer, so the parser keeps it as a double and the sign survives.
    const auto f32_text_zero{ParseTaggedText("f32", "-0.0")};
    ASSERT_TRUE(f32_text_zero);
    EXPECT_TRUE(std::signbit(std::get<float>(f32_text_zero->GetValue())));

    const auto u32_zero{ParseTaggedText("u32", "-0")};
    ASSERT_FALSE(u32_zero.has_value());
    EXPECT_EQ(u32_zero.error(), ErrorCode::kInvalidValueType);
}

namespace
{

template <typename T>
void ExpectJsonRoundTrip(const T value)
{
    const KvsValue kvs_value{value};
    const auto any{KvsValueToAny(kvs_value)};
    ASSERT_TRUE(any);

    score::json::JsonWriter writer;
    const auto buffer{writer.ToBuffer(any.value())};
    ASSERT_TRUE(buffer);

    const score::json::JsonParser parser;
    const auto parsed{parser.FromBuffer(buffer.value())};
    ASSERT_TRUE(parsed) << buffer.value();

    const auto result{AnyToKvsValue(parsed.value())};
    ASSERT_TRUE(result) << buffer.value();
    ASSERT_EQ(result->GetType(), kvs_value.GetType()) << buffer.value();
    EXPECT_EQ(std::get<T>(result->GetValue()), value) << buffer.value();
}

template <typename T>
void ExpectJsonRoundTripBoundaries()
{
    ExpectJsonRoundTrip<T>(std::numeric_limits<T>::lowest());
    ExpectJsonRoundTrip<T>(static_cast<T>(std::numeric_limits<T>::lowest() / 3));
    ExpectJsonRoundTrip<T>(T{0});
    ExpectJsonRoundTrip<T>(static_cast<T>(std::numeric_limits<T>::max() / 3));
    ExpectJsonRoundTrip<T>(std::numeric_limits<T>::max());
}

}  // namespace

TEST(kvs_any_to_kvsvalue, numeric_values_json_round_trip)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_data_types, comp_req__kvs__value_serialize");
    RecordProperty("Description",
                   "Check that minimum, in-range and maximum values of every numeric type are preserved when "
                   "serialized to JSON text and parsed back.");

    ExpectJsonRoundTripBoundaries<uint8_t>();
    ExpectJsonRoundTripBoundaries<uint16_t>();
    ExpectJsonRoundTripBoundaries<uint32_t>();
    ExpectJsonRoundTripBoundaries<uint64_t>();
    ExpectJsonRoundTripBoundaries<int8_t>();
    ExpectJsonRoundTripBoundaries<int16_t>();
    ExpectJsonRoundTripBoundaries<int32_t>();
    ExpectJsonRoundTripBoundaries<int64_t>();
    ExpectJsonRoundTripBoundaries<float>();
#if !defined(__QNXNTO__)
    // TODO: Re-enable on QNX after https://github.com/eclipse-score/baselibs/issues/894 is fixed.
    ExpectJsonRoundTripBoundaries<double>();
#endif
    ExpectJsonRoundTrip<float>(std::numeric_limits<float>::min());
    ExpectJsonRoundTrip<float>(0.1F);
    // Written as shortest float text this value would load one ULP off, because the parser rounds it to a double first.
    ExpectJsonRoundTrip<float>(7.038531e-26F);
    ExpectJsonRoundTrip<float>(-7.038531e-26F);
    ExpectJsonRoundTrip<double>(std::numeric_limits<double>::min());
    ExpectJsonRoundTrip<double>(0.1);
}

TEST(kvs_any_to_kvsvalue, numeric_values_parse_rust_written_text)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "boundary-values");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_data_types, comp_req__kvs__value_serialize");
    RecordProperty("Description",
                   "Check that boundary numeric values written by the Rust implementation (plain decimal notation "
                   "without exponent) are parsed to the expected tagged values.");

    const auto parse{ParseTaggedText};

    const auto u32_max{parse("u32", "4294967295")};
    ASSERT_TRUE(u32_max);
    EXPECT_EQ(std::get<uint32_t>(u32_max->GetValue()), std::numeric_limits<uint32_t>::max());

    const auto i64_min{parse("i64", "-9223372036854775808")};
    ASSERT_TRUE(i64_min);
    EXPECT_EQ(std::get<int64_t>(i64_min->GetValue()), std::numeric_limits<int64_t>::min());

    // Integral JSON numbers are parsed into integer types, which must still convert to floating point.
    const std::pair<const char*, float> f32_integers[]{
        {"100000", 100000.0F}, {"-123456", -123456.0F}, {"16777216", 16777216.0F}, {"16777217", 16777216.0F}};
    for (const auto& [text, expected] : f32_integers)
    {
        SCOPED_TRACE(text);
        const auto result{parse("f32", text)};
        ASSERT_TRUE(result);
        EXPECT_EQ(std::get<float>(result->GetValue()), expected);
    }

    const auto f64_integer{parse("f64", "4294967296")};
    ASSERT_TRUE(f64_integer);
    EXPECT_EQ(std::get<double>(f64_integer->GetValue()), 4294967296.0);

    const auto f32_max{parse("f32", "340282346638528860000000000000000000000")};
    ASSERT_TRUE(f32_max);
    EXPECT_EQ(std::get<float>(f32_max->GetValue()), std::numeric_limits<float>::max());

    const auto f32_lowest{parse("f32", "-340282346638528860000000000000000000000")};
    ASSERT_TRUE(f32_lowest);
    EXPECT_EQ(std::get<float>(f32_lowest->GetValue()), std::numeric_limits<float>::lowest());

#if !defined(__QNXNTO__)
    // TODO: Re-enable on QNX after https://github.com/eclipse-score/baselibs/issues/894 is fixed.
    const auto f64_max{parse("f64", "17976931348623157" + std::string(292U, '0'))};
    ASSERT_TRUE(f64_max);
    EXPECT_EQ(std::get<double>(f64_max->GetValue()), std::numeric_limits<double>::max());
#endif
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_string)
{
    auto result{AnyToKvsValue(MakeTaggedValue("str", score::json::Any(std::string("test"))))};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().GetType(), KvsValue::Type::kString);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_null)
{
    auto result{AnyToKvsValue(MakeTaggedValue("null", score::json::Any(score::json::Null())))};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().GetType(), KvsValue::Type::kNull);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_array)
{
    score::json::List list;
    list.push_back(MakeTaggedValue("bool", score::json::Any(true)));
    list.push_back(MakeTaggedValue("f64", score::json::Any(1.1)));
    list.push_back(MakeTaggedValue("str", score::json::Any(std::string("test"))));

    auto result{AnyToKvsValue(MakeTaggedValue("arr", score::json::Any(std::move(list))))};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().GetType(), KvsValue::Type::kArray);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_object)
{
    score::json::Object combined_obj;
    combined_obj.emplace("flag", MakeTaggedValue("bool", score::json::Any(true)));
    combined_obj.emplace("count", MakeTaggedValue("f64", score::json::Any(42.0)));

    auto result{AnyToKvsValue(MakeTaggedValue("obj", score::json::Any(std::move(combined_obj))))};
    ASSERT_TRUE(result);
    EXPECT_EQ(result.value().GetType(), KvsValue::Type::kObject);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_object_over_length_key_accepted)
{
    // comp_req__kvs__key_length applies to root keys only.
    // Keys of nested objects are value data.
    const std::string long_key(kMaxKeyLengthBytes + 1U, 'x');

    score::json::Object combined_obj;
    combined_obj.emplace(long_key, MakeTaggedValue("i32", score::json::Any(1)));

    auto result{AnyToKvsValue(MakeTaggedValue("obj", score::json::Any(std::move(combined_obj))))};
    ASSERT_TRUE(result);
    ASSERT_EQ(result.value().GetType(), KvsValue::Type::kObject);
    const auto& result_obj{std::get<KvsValue::Object>(result.value().GetValue())};
    ASSERT_EQ(result_obj.size(), 1U);
    EXPECT_EQ(result_obj[0].first, long_key);
    ASSERT_EQ(result_obj[0].second.GetType(), KvsValue::Type::kI32);
    EXPECT_EQ(std::get<int32_t>(result_obj[0].second.GetValue()), 1);
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_format_invalid)
{
    score::json::Object obj_type;
    obj_type.emplace("invalid", score::json::Any(std::string("bool")));
    obj_type.emplace("v", score::json::Any(true));
    ExpectInvalid(score::json::Any(std::move(obj_type)));

    score::json::Object obj_value;
    obj_value.emplace("t", score::json::Any(std::string("bool")));
    obj_value.emplace("invalid", score::json::Any(true));
    ExpectInvalid(score::json::Any(std::move(obj_value)));
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_no_object)
{
    ExpectInvalid(score::json::Any(true));
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_type_no_string)
{
    score::json::Object obj;
    // Not a string.
    obj.emplace("t", score::json::Any(42.0));
    obj.emplace("v", score::json::Any(true));
    ExpectInvalid(score::json::Any(std::move(obj)));
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_type_invalid)
{
    ExpectInvalidTaggedValue("invalid", score::json::Any(true));
}

TEST(kvs_any_to_kvsvalue, numeric_values_reject_strings)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_serialize");
    RecordProperty("Description", "Check that every numeric type tag rejects a string value.");

    for (const auto* tag : {"i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64"})
    {
        ExpectInvalidTaggedValue(tag, score::json::Any("invalid"));
    }
}

TEST(kvs_any_to_kvsvalue, non_numeric_values_reject_numbers)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_serialize");
    RecordProperty("Description", "Check that every non-numeric type tag rejects a numeric value.");

    for (const auto* tag : {"bool", "str", "null", "arr", "obj"})
    {
        ExpectInvalidTaggedValue(tag, score::json::Any(42.0));
    }
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_array_with_invalid_element)
{
    score::json::List list;

    list.push_back(MakeTaggedValue("bool", score::json::Any(true)));
    list.push_back(MakeTaggedValue("InvalidType", score::json::Any(std::string("test"))));

    ExpectInvalidTaggedValue("arr", score::json::Any(std::move(list)));
}

TEST(kvs_any_to_kvsvalue, any_to_kvsvalue_object_with_invalid_value)
{
    score::json::Object value_obj;
    value_obj.emplace("flag", MakeTaggedValue("bool", score::json::Any(true)));
    value_obj.emplace("count", MakeTaggedValue("InvalidType", score::json::Any(42.0)));

    ExpectInvalidTaggedValue("obj", score::json::Any(std::move(value_obj)));
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_null)
{
    KvsValue null_value{nullptr};
    auto result{KvsValueToAny(null_value)};
    ASSERT_TRUE(result);
    const auto& obj{result.value().As<score::json::Object>().value().get()};
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "null");
    EXPECT_TRUE(obj.at("v").As<score::json::Null>().has_value());
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_boolean)
{
    ExpectTaggedValue(KvsValue{true}, "bool", true);
}

TEST(kvs_kvsvalue_to_any, numeric_values_preserve_tags_and_values)
{
    RecordProperty("TestType", "interface-test");
    RecordProperty("DerivationTechnique", "equivalence-classes");
    RecordProperty("PartiallyVerifies", "comp_req__kvs__value_serialize");
    RecordProperty("Description",
                   "Check that every numeric KvsValue variant serializes with its expected tag and value.");

    ExpectTaggedValue(KvsValue{static_cast<uint8_t>(42)}, "u8", static_cast<uint8_t>(42));
    ExpectTaggedValue(KvsValue{static_cast<uint16_t>(42)}, "u16", static_cast<uint16_t>(42));
    ExpectTaggedValue(KvsValue{static_cast<uint32_t>(42)}, "u32", static_cast<uint32_t>(42));
    ExpectTaggedValue(KvsValue{static_cast<uint64_t>(42)}, "u64", static_cast<uint64_t>(42));

    ExpectTaggedValue(KvsValue{static_cast<int8_t>(42)}, "i8", static_cast<int8_t>(42));
    ExpectTaggedValue(KvsValue{static_cast<int16_t>(42)}, "i16", static_cast<int16_t>(42));
    ExpectTaggedValue(KvsValue{static_cast<int32_t>(42)}, "i32", static_cast<int32_t>(42));
    ExpectTaggedValue(KvsValue{static_cast<int64_t>(42)}, "i64", static_cast<int64_t>(42));

    ExpectTaggedValue(KvsValue{static_cast<float>(42)}, "f32", 42.0F);
    ExpectTaggedValue(KvsValue{static_cast<double>(42)}, "f64", 42.0);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_string)
{
    std::string str{"test"};
    ExpectTaggedValue(KvsValue{str}, "str", str);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_array)
{
    KvsValue::Array array;
    array.push_back(KvsValue(true));
    array.push_back(KvsValue(1.1));
    array.push_back(KvsValue(std::string("test")));
    KvsValue array_value{array};
    auto result{KvsValueToAny(array_value)};
    ASSERT_TRUE(result);
    const auto& obj{result.value().As<score::json::Object>().value().get()};
    EXPECT_EQ(obj.at("t").As<std::string>().value().get(), "arr");
    const auto& list{obj.at("v").As<score::json::List>().value().get()};
    EXPECT_EQ(list.size(), 3);

    const auto& element0{list[0].As<score::json::Object>().value().get()};
    EXPECT_EQ(element0.at("t").As<std::string>().value().get(), "bool");
    EXPECT_EQ(element0.at("v").As<bool>().value(), true);

    const auto& element1{list[1].As<score::json::Object>().value().get()};
    EXPECT_EQ(element1.at("t").As<std::string>().value().get(), "f64");
    EXPECT_EQ(element1.at("v").As<double>().value(), 1.1);

    const auto& element2{list[2].As<score::json::Object>().value().get()};
    EXPECT_EQ(element2.at("t").As<std::string>().value().get(), "str");
    EXPECT_EQ(element2.at("v").As<std::string>().value().get(), "test");
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_object)
{
    KvsValue::Object obj;
    // Boolean.
    obj.emplace_back("flag", KvsValue(true));
    // F64.
    obj.emplace_back("count", KvsValue(42.0));
    KvsValue obj_value{obj};

    auto result{KvsValueToAny(obj_value)};
    ASSERT_TRUE(result);

    const auto& outer_obj{result.value().As<score::json::Object>().value().get()};
    EXPECT_EQ(outer_obj.at("t").As<std::string>().value().get(), "obj");

    const auto& inner_obj{outer_obj.at("v").As<score::json::Object>().value().get()};

    const auto& flag_entry{inner_obj.at("flag").As<score::json::Object>().value().get()};
    EXPECT_EQ(flag_entry.at("t").As<std::string>().value().get(), "bool");
    EXPECT_EQ(flag_entry.at("v").As<bool>().value(), true);

    const auto& count_entry{inner_obj.at("count").As<score::json::Object>().value().get()};
    EXPECT_EQ(count_entry.at("t").As<std::string>().value().get(), "f64");
    EXPECT_EQ(count_entry.at("v").As<double>().value(), 42.0);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_object_duplicate_key)
{
    KvsValue::Object obj{{"key", KvsValue(int32_t{1})}, {"key", KvsValue(int32_t{2})}};
    auto result{KvsValueToAny(KvsValue(obj))};
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kInvalidValueType);

    // Duplicate key in an object nested in an array.
    KvsValue::Array array{KvsValue(std::move(obj))};
    result = KvsValueToAny(KvsValue(std::move(array)));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kInvalidValueType);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_map)
{
    // A map is serialized directly, without wrapping it in a KvsValue.
    KvsMap map;
    map.emplace("flag", KvsValue(true));
    map.emplace("name", KvsValue(std::string("test")));
    map.emplace("nested", KvsValue(KvsValue::Object{{"count", KvsValue(int32_t{7})}}));

    auto result{KvsValueToAny(map)};
    ASSERT_TRUE(result);

    const auto& outer_obj{result.value().As<score::json::Object>().value().get()};
    EXPECT_EQ(outer_obj.at("t").As<std::string>().value().get(), "obj");

    const auto& inner_obj{outer_obj.at("v").As<score::json::Object>().value().get()};
    EXPECT_EQ(inner_obj.size(), 3U);

    const auto& flag_entry{inner_obj.at("flag").As<score::json::Object>().value().get()};
    EXPECT_EQ(flag_entry.at("t").As<std::string>().value().get(), "bool");
    EXPECT_EQ(flag_entry.at("v").As<bool>().value(), true);

    const auto& name_entry{inner_obj.at("name").As<score::json::Object>().value().get()};
    EXPECT_EQ(name_entry.at("t").As<std::string>().value().get(), "str");
    EXPECT_EQ(name_entry.at("v").As<std::string>().value().get(), "test");

    const auto& nested_entry{inner_obj.at("nested").As<score::json::Object>().value().get()};
    EXPECT_EQ(nested_entry.at("t").As<std::string>().value().get(), "obj");
    const auto& nested_obj{nested_entry.at("v").As<score::json::Object>().value().get()};
    const auto& count_entry{nested_obj.at("count").As<score::json::Object>().value().get()};
    EXPECT_EQ(count_entry.at("t").As<std::string>().value().get(), "i32");
    EXPECT_EQ(count_entry.at("v").As<int32_t>().value(), 7);
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_map_empty)
{
    KvsMap map;
    auto result{KvsValueToAny(map)};
    ASSERT_TRUE(result);

    const auto& outer_obj{result.value().As<score::json::Object>().value().get()};
    EXPECT_EQ(outer_obj.at("t").As<std::string>().value().get(), "obj");
    EXPECT_TRUE(outer_obj.at("v").As<score::json::Object>().value().get().empty());
}

TEST(kvs_kvsvalue_to_any, kvsvalue_to_any_map_invalid)
{
    KvsMap map;
    map.emplace("valid", KvsValue(42.0));
    map.emplace("invalid", KvsValue(KvsValue::Object{{"key", KvsValue(true)}, {"key", KvsValue(false)}}));

    auto result{KvsValueToAny(map)};
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ErrorCode::kInvalidValueType);
}

TEST(kvs_has_unique_keys, has_unique_keys_scalar)
{
    EXPECT_TRUE(HasUniqueKeys(KvsValue(int32_t{1})));
    EXPECT_TRUE(HasUniqueKeys(KvsValue("text")));
    EXPECT_TRUE(HasUniqueKeys(KvsValue(nullptr)));
}

TEST(kvs_has_unique_keys, has_unique_keys_unique)
{
    KvsValue::Object inner{{"a", KvsValue(true)}, {"b", KvsValue(false)}};
    KvsValue::Array array{KvsValue(inner), KvsValue(inner)};
    KvsValue::Object outer{{"a", KvsValue(std::move(inner))}, {"b", KvsValue(std::move(array))}};
    EXPECT_TRUE(HasUniqueKeys(KvsValue(std::move(outer))));
}

TEST(kvs_has_unique_keys, has_unique_keys_duplicate)
{
    KvsValue::Object obj{{"a", KvsValue(true)}, {"a", KvsValue(false)}};
    EXPECT_FALSE(HasUniqueKeys(KvsValue(obj)));
}

TEST(kvs_has_unique_keys, has_unique_keys_duplicate_nested_in_object)
{
    KvsValue::Object inner{{"a", KvsValue(true)}, {"a", KvsValue(false)}};
    KvsValue::Object outer{{"inner", KvsValue(std::move(inner))}};
    EXPECT_FALSE(HasUniqueKeys(KvsValue(std::move(outer))));
}

TEST(kvs_has_unique_keys, has_unique_keys_duplicate_nested_in_array)
{
    KvsValue::Object inner{{"a", KvsValue(true)}, {"a", KvsValue(false)}};
    KvsValue::Array array{KvsValue(int32_t{0}), KvsValue(std::move(inner))};
    EXPECT_FALSE(HasUniqueKeys(KvsValue(std::move(array))));
}

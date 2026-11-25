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

#include "score/kvs/internal/kvs_helper.h"

#include "score/utility.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>

namespace score::kvs
{

namespace
{

/// Narrows a double to a finite value of floating point type `T`.
/// Returns no value if the double does not round to a finite `T`.
template <typename T>
std::optional<T> NarrowToFinite(const double value) noexcept
{
    // codeql::misra_deviation_next_line(kvs-bool-ref-load)
    static_assert(std::is_floating_point_v<T>);
    if (!std::isfinite(value))
    {
        return std::nullopt;
    }

    // codeql::misra_deviation_next_line(kvs-bool-ref-load)
    if constexpr (std::is_same_v<T, double>)
    {
        return value;
    }
    else
    {
        // `T::max()` written in shortest round-trip form of `T` parses to a double slightly above it that still rounds
        // back to it. Values below `max + ulp / 2` are rounded to `max`, values at or above it are rounded to infinity.
        const double max{static_cast<double>(std::numeric_limits<T>::max())};
        const double max_ulp{
            max - static_cast<double>(std::nextafter(std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest()))};
        if (std::abs(value) >= (max + (max_ulp / 2.0)))
        {
            return std::nullopt;
        }
        return static_cast<T>(value);
    }
}

/// Converts a stored JSON number of any arithmetic type to double.
/// A named function object is used because a captureless lambda has an implicit conversion to a function pointer.
struct ToDouble final
{
    template <typename Stored>
    double operator()(const Stored stored) const noexcept
    {
        return static_cast<double>(stored);
    }
};

template <typename T>
score::Result<KvsValue> NumberToKvsValue(const score::json::Any& value) noexcept
{
    const auto number{value.As<score::json::Number>()};
    if (!number.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }

    // codeql::misra_deviation_next_line(kvs-bool-ref-load)
    if constexpr (std::is_floating_point_v<T>)
    {
        // Integers beyond 2^53 are rounded to the nearest double.
        const double as_double{number->get().Visit(ToDouble{})};
        const std::optional<T> narrowed{NarrowToFinite<T>(as_double)};
        // codeql::misra_deviation_next_line(kvs-bool-ref-load)
        if (!narrowed)
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(*narrowed);
    }
    else
    {
        const auto converted{number->get().As<T>()};
        // codeql::misra_deviation_next_line(kvs-bool-ref-load)
        if (!converted)
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(*converted);
    }
}

/// Builds the `{"t": <type_tag>, "v": <value>}` wrapper used for every stored value.
score::json::Any MakeTagged(const std::string_view type_tag, score::json::Any value) noexcept
{
    score::json::Object obj;
    score::cpp::ignore = obj.emplace("t", score::json::Any(std::string(type_tag)));
    score::cpp::ignore = obj.emplace("v", std::move(value));
    return score::json::Any(std::move(obj));
}

template <typename T>
score::Result<score::json::Any> TaggedValueToAny(const KvsValue& value, const std::string_view type_tag) noexcept
{
    const auto* const scalar{std::get_if<T>(&value.GetValue())};
    if (scalar == nullptr)
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }
    // codeql::misra_deviation_next_line(kvs-bool-ref-load)
    if constexpr (std::is_same_v<T, float>)
    {
        // The parser rounds JSON text to a double before it can be narrowed, so shortest float text can load one ULP
        // off. The exact double text of the widened float narrows back to it, which is also how Rust writes `f32`.
        return MakeTagged(type_tag, score::json::Any(static_cast<double>(*scalar)));
    }
    else
    {
        return MakeTagged(type_tag, score::json::Any(*scalar));
    }
}

/// Converts object members to `{"t": "obj", "v": <object>}`, failing on a duplicate key.
template <typename Members>
score::Result<score::json::Any> ObjectToAny(const Members& members) noexcept
{
    score::json::Object inner_obj;
    for (const auto& member : members)
    {
        // codeql::misra_deviation_next_line(kvs-recursive-value)
        auto converted{KvsValueToAny(member.second)};
        // codeql::misra_deviation_next_line(kvs-bool-ref-load)
        if (!converted)
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        // codeql::misra_deviation_next_line(kvs-bool-ref-load)
        if (!inner_obj.emplace(member.first, std::move(*converted)).second)
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
    }

    return MakeTagged("obj", score::json::Any(std::move(inner_obj)));
}

}  // namespace

uint32_t ParseHashAdler32(std::istream& in) noexcept
{
    std::array<char, kHashSize> buffer{};
    score::cpp::ignore = in.read(buffer.data(), buffer.size());

    uint32_t value{0U};
    for (const char byte : buffer)
    {
        value = (value << 8U) | static_cast<uint32_t>(std::char_traits<char>::to_int_type(byte));
    }
    return value;
}

// Optimized version: processes data in blocks to reduce modulo operations.
uint32_t CalculateHashAdler32(const std::string& data) noexcept
{
    constexpr size_t kAdler32Nmax{5552U};
    constexpr uint32_t kAdler32Base{65521U};
    uint32_t a{1U};
    uint32_t b{0U};
    size_t length{data.size()};
    size_t i{0U};

    // Process in blocks of 5552 bytes (as recommended for Adler-32).
    while (length > 0)
    {
        const size_t chunk_length{(length > kAdler32Nmax) ? kAdler32Nmax : length};
        length -= chunk_length;
        for (size_t j{0U}; j < chunk_length; ++j)
        {
            a += static_cast<uint32_t>(std::char_traits<char>::to_int_type(data[i + j]));
            b += a;
        }
        i += chunk_length;
        a %= kAdler32Base;
        b %= kAdler32Base;
    }
    return (b << 16) | a;
}

std::array<uint8_t, kHashSize> GetHashBytesAdler32(const uint32_t hash) noexcept
{
    std::array<uint8_t, kHashSize> value{static_cast<uint8_t>((hash >> 24) & 0xFFU),
                                         static_cast<uint8_t>((hash >> 16) & 0xFFU),
                                         static_cast<uint8_t>((hash >> 8) & 0xFFU),
                                         static_cast<uint8_t>(hash & 0xFFU)};
    return value;
}

std::array<uint8_t, kHashSize> GetHashBytes(const std::string& data) noexcept
{
    const uint32_t hash{CalculateHashAdler32(data)};
    std::array<uint8_t, kHashSize> value{GetHashBytesAdler32(hash)};
    return value;
}

bool CheckHash(const std::string& data_calculate, std::istream& data_parse) noexcept
{
    const uint32_t calculated_hash{CalculateHashAdler32(data_calculate)};
    const uint32_t parsed_hash{ParseHashAdler32(data_parse)};
    return calculated_hash == parsed_hash;
}

score::Result<KvsValue> AnyToKvsValue(const score::json::Any& any) noexcept
{
    const auto obj_result{any.As<score::json::Object>()};
    if (!obj_result.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }

    const auto& obj_any{obj_result->get()};
    const auto type{obj_any.find("t")};
    const auto value{obj_any.find("v")};
    if (type == obj_any.end() || value == obj_any.end())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }

    const auto type_str{type->second.As<std::string>()};
    if (!type_str.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }

    const std::string_view type_tag{type_str->get()};
    const score::json::Any& value_any{value->second};

    if (type_tag == "u8")
    {
        return NumberToKvsValue<uint8_t>(value_any);
    }
    else if (type_tag == "u16")
    {
        return NumberToKvsValue<uint16_t>(value_any);
    }
    else if (type_tag == "u32")
    {
        return NumberToKvsValue<uint32_t>(value_any);
    }
    else if (type_tag == "u64")
    {
        return NumberToKvsValue<uint64_t>(value_any);
    }
    else if (type_tag == "i8")
    {
        return NumberToKvsValue<int8_t>(value_any);
    }
    else if (type_tag == "i16")
    {
        return NumberToKvsValue<int16_t>(value_any);
    }
    else if (type_tag == "i32")
    {
        return NumberToKvsValue<int32_t>(value_any);
    }
    else if (type_tag == "i64")
    {
        return NumberToKvsValue<int64_t>(value_any);
    }
    else if (type_tag == "f32")
    {
        return NumberToKvsValue<float>(value_any);
    }
    else if (type_tag == "f64")
    {
        return NumberToKvsValue<double>(value_any);
    }
    else if (type_tag == "bool")
    {
        const auto boolean{value_any.As<bool>()};
        if (!boolean.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        // codeql::misra_deviation_next_line(kvs-bool-ref-load)
        return KvsValue(*boolean);
    }
    else if (type_tag == "str")
    {
        const auto str{value_any.As<std::string>()};
        if (!str.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(str->get());
    }
    else if (type_tag == "null")
    {
        if (!value_any.As<score::json::Null>().has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(nullptr);
    }
    else if (type_tag == "arr")
    {
        const auto list{value_any.As<score::json::List>()};
        if (!list.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        const score::json::List& elements{list->get()};
        KvsValue::Array arr;
        for (const auto& element : elements)
        {
            // codeql::misra_deviation_next_line(kvs-recursive-value)
            auto converted{AnyToKvsValue(element)};
            if (!converted)
            {
                return score::MakeUnexpected(ErrorCode::kInvalidValueType);
            }
            score::cpp::ignore = arr.emplace_back(std::move(*converted));
        }
        return KvsValue(std::move(arr));
    }
    else if (type_tag == "obj")
    {
        const auto obj{value_any.As<score::json::Object>()};
        if (!obj.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        KvsValue::Object map;
        const score::json::Object& members{obj->get()};
        for (const auto& member : members)
        {
            // codeql::misra_deviation_next_line(kvs-recursive-value)
            auto converted{AnyToKvsValue(member.second)};
            if (!converted)
            {
                return score::MakeUnexpected(ErrorCode::kInvalidValueType);
            }
            score::cpp::ignore = map.emplace_back(std::string(member.first.GetAsStringView()), std::move(*converted));
        }
        return KvsValue(std::move(map));
    }
    else
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }
}

score::Result<score::json::Any> KvsValueToAny(const KvsValue& kvs_value) noexcept
{
    switch (kvs_value.GetType())
    {
        case KvsValue::Type::kU8:
            return TaggedValueToAny<uint8_t>(kvs_value, "u8");
        case KvsValue::Type::kU16:
            return TaggedValueToAny<uint16_t>(kvs_value, "u16");
        case KvsValue::Type::kU32:
            return TaggedValueToAny<uint32_t>(kvs_value, "u32");
        case KvsValue::Type::kU64:
            return TaggedValueToAny<uint64_t>(kvs_value, "u64");
        case KvsValue::Type::kI8:
            return TaggedValueToAny<int8_t>(kvs_value, "i8");
        case KvsValue::Type::kI16:
            return TaggedValueToAny<int16_t>(kvs_value, "i16");
        case KvsValue::Type::kI32:
            return TaggedValueToAny<int32_t>(kvs_value, "i32");
        case KvsValue::Type::kI64:
            return TaggedValueToAny<int64_t>(kvs_value, "i64");
        case KvsValue::Type::kF32:
            return TaggedValueToAny<float>(kvs_value, "f32");
        case KvsValue::Type::kF64:
            return TaggedValueToAny<double>(kvs_value, "f64");
        case KvsValue::Type::kBoolean:
            return TaggedValueToAny<bool>(kvs_value, "bool");
        case KvsValue::Type::kString:
            return TaggedValueToAny<std::string>(kvs_value, "str");
        case KvsValue::Type::kNull:
        {
            return MakeTagged("null", score::json::Any(score::json::Null{}));
        }
        case KvsValue::Type::kArray:
        {
            const auto* value = std::get_if<KvsValue::Array>(&kvs_value.GetValue());
            if (value == nullptr)
            {
                return score::MakeUnexpected(ErrorCode::kInvalidValueType);
            }
            score::json::List list;
            for (const auto& element : *value)
            {
                // codeql::misra_deviation_next_line(kvs-recursive-value)
                auto converted{KvsValueToAny(element)};
                if (!converted)
                {
                    return score::MakeUnexpected(ErrorCode::kInvalidValueType);
                }
                list.push_back(std::move(*converted));
            }
            return MakeTagged("arr", score::json::Any(std::move(list)));
        }
        case KvsValue::Type::kObject:
        {
            const auto* value = std::get_if<KvsValue::Object>(&kvs_value.GetValue());
            if (value == nullptr)
            {
                return score::MakeUnexpected(ErrorCode::kInvalidValueType);
            }
            // codeql::misra_deviation_next_line(kvs-recursive-value)
            return KvsValueToAny(*value);
        }
        default:
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
    }
}

score::Result<score::json::Any> KvsValueToAny(const KvsValue::Object& object) noexcept
{
    // codeql::misra_deviation_next_line(kvs-recursive-value)
    return ObjectToAny(object);
}

score::Result<score::json::Any> KvsValueToAny(const KvsMap& map) noexcept
{
    return ObjectToAny(map);
}

bool HasUniqueKeys(const KvsValue& value) noexcept
{
    switch (value.GetType())
    {
        case KvsValue::Type::kArray:
        {
            const auto* array{std::get_if<KvsValue::Array>(&value.GetValue())};
            if (array == nullptr)
            {
                return false;
            }
            return std::all_of(array->cbegin(), array->cend(), HasUniqueKeys);
        }
        case KvsValue::Type::kObject:
        {
            const auto* object{std::get_if<KvsValue::Object>(&value.GetValue())};
            if (object == nullptr)
            {
                return false;
            }
            std::unordered_set<std::string_view> keys;
            keys.reserve(object->size());
            for (const auto& entry : *object)
            {
                // codeql::misra_deviation_next_line(kvs-recursive-value)
                if ((!keys.insert(entry.first).second) || (!HasUniqueKeys(entry.second)))
                {
                    return false;
                }
            }
            return true;
        }
        case KvsValue::Type::kU8:
        case KvsValue::Type::kU16:
        case KvsValue::Type::kU32:
        case KvsValue::Type::kU64:
        case KvsValue::Type::kI8:
        case KvsValue::Type::kI16:
        case KvsValue::Type::kI32:
        case KvsValue::Type::kI64:
        case KvsValue::Type::kF32:
        case KvsValue::Type::kF64:
        case KvsValue::Type::kBoolean:
        case KvsValue::Type::kString:
        case KvsValue::Type::kNull:
        {
            // Scalars have no keys.
            return true;
        }
        default:
        {
            return true;
        }
    }
}

}  // namespace score::kvs

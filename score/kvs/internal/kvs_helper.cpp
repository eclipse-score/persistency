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

#include <algorithm>
#include <string_view>
#include <unordered_set>

namespace score::kvs
{

namespace
{

/// Converts object members to `{"t": "obj", "v": <object>}`, failing on a duplicate key.
template <typename Members>
score::Result<score::json::Any> ObjectToAny(const Members& members) noexcept
{
    score::json::Object inner_obj;
    for (const auto& [key, value] : members)
    {
        auto converted{KvsValueToAny(value)};
        if (!converted)
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        if (!inner_obj.emplace(key, std::move(converted.value())).second)
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
    }

    score::json::Object obj;
    obj.emplace("t", score::json::Any(std::string("obj")));
    obj.emplace("v", score::json::Any(std::move(inner_obj)));
    return score::json::Any(std::move(obj));
}

}  // namespace

uint32_t ParseHashAdler32(std::istream& in) noexcept
{
    std::array<uint8_t, kHashSize> buffer{};
    in.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    const uint32_t value{(static_cast<uint32_t>(buffer[0]) << 24) | (static_cast<uint32_t>(buffer[1]) << 16) |
                         (static_cast<uint32_t>(buffer[2]) << 8) | static_cast<uint32_t>(buffer[3])};
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
        const size_t chunk_length{length > kAdler32Nmax ? kAdler32Nmax : length};
        length -= chunk_length;
        for (size_t j{0U}; j < chunk_length; ++j, ++i)
        {
            a += static_cast<unsigned char>(data[i]);
            b += a;
        }
        a %= kAdler32Base;
        b %= kAdler32Base;
    }
    return (b << 16) | a;
}

std::array<uint8_t, kHashSize> GetHashBytesAdler32(const uint32_t hash) noexcept
{
    std::array<uint8_t, kHashSize> value{static_cast<uint8_t>((hash >> 24) & 0xFF),
                                         static_cast<uint8_t>((hash >> 16) & 0xFF),
                                         static_cast<uint8_t>((hash >> 8) & 0xFF),
                                         static_cast<uint8_t>((hash) & 0xFF)};
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

    const auto& obj_any{obj_result.value().get()};
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

    const std::string_view type_tag{type_str.value().get()};
    const score::json::Any& value_any{value->second};

    if (type_tag == "i32")
    {
        const auto number{value_any.As<int32_t>()};
        if (!number.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(static_cast<int32_t>(number.value()));
    }
    else if (type_tag == "u32")
    {
        const auto number{value_any.As<uint32_t>()};
        if (!number.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(static_cast<uint32_t>(number.value()));
    }
    else if (type_tag == "i64")
    {
        const auto number{value_any.As<int64_t>()};
        if (!number.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(static_cast<int64_t>(number.value()));
    }
    else if (type_tag == "u64")
    {
        const auto number{value_any.As<uint64_t>()};
        if (!number.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(static_cast<uint64_t>(number.value()));
    }
    else if (type_tag == "f64")
    {
        const auto number{value_any.As<double>()};
        if (!number.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(number.value());
    }
    else if (type_tag == "bool")
    {
        const auto boolean{value_any.As<bool>()};
        if (!boolean.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(boolean.value());
    }
    else if (type_tag == "str")
    {
        const auto str{value_any.As<std::string>()};
        if (!str.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(str.value().get());
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
        KvsValue::Array arr;
        for (const auto& element : list.value().get())
        {
            auto converted{AnyToKvsValue(element)};
            if (!converted)
            {
                return score::MakeUnexpected(ErrorCode::kInvalidValueType);
            }
            arr.emplace_back(std::move(converted.value()));
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
        for (const auto& [key, member_any] : obj.value().get())
        {
            auto converted{AnyToKvsValue(member_any)};
            if (!converted)
            {
                return score::MakeUnexpected(ErrorCode::kInvalidValueType);
            }
            map.emplace_back(std::string(key.GetAsStringView()), std::move(converted.value()));
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
    score::json::Object obj;
    switch (kvs_value.GetType())
    {
        case KvsValue::Type::kI32:
        {
            obj.emplace("t", score::json::Any(std::string("i32")));
            obj.emplace("v", score::json::Any(static_cast<int32_t>(std::get<int32_t>(kvs_value.GetValue()))));
            break;
        }
        case KvsValue::Type::kU32:
        {
            obj.emplace("t", score::json::Any(std::string("u32")));
            obj.emplace("v", score::json::Any(static_cast<uint32_t>(std::get<uint32_t>(kvs_value.GetValue()))));
            break;
        }
        case KvsValue::Type::kI64:
        {
            obj.emplace("t", score::json::Any(std::string("i64")));
            obj.emplace("v", score::json::Any(static_cast<int64_t>(std::get<int64_t>(kvs_value.GetValue()))));
            break;
        }
        case KvsValue::Type::kU64:
        {
            obj.emplace("t", score::json::Any(std::string("u64")));
            obj.emplace("v", score::json::Any(static_cast<uint64_t>(std::get<uint64_t>(kvs_value.GetValue()))));
            break;
        }
        case KvsValue::Type::kF64:
        {
            obj.emplace("t", score::json::Any(std::string("f64")));
            obj.emplace("v", score::json::Any(std::get<double>(kvs_value.GetValue())));
            break;
        }
        case KvsValue::Type::kBoolean:
        {
            obj.emplace("t", score::json::Any(std::string("bool")));
            obj.emplace("v", score::json::Any(std::get<bool>(kvs_value.GetValue())));
            break;
        }
        case KvsValue::Type::kString:
        {
            obj.emplace("t", score::json::Any(std::string("str")));
            obj.emplace("v", score::json::Any(std::get<std::string>(kvs_value.GetValue())));
            break;
        }
        case KvsValue::Type::kNull:
        {
            obj.emplace("t", score::json::Any(std::string("null")));
            obj.emplace("v", score::json::Any(score::json::Null{}));
            break;
        }
        case KvsValue::Type::kArray:
        {
            obj.emplace("t", score::json::Any(std::string("arr")));
            score::json::List list;
            for (const auto& element : std::get<KvsValue::Array>(kvs_value.GetValue()))
            {
                auto converted{KvsValueToAny(element)};
                if (!converted)
                {
                    return score::MakeUnexpected(ErrorCode::kInvalidValueType);
                }
                list.push_back(std::move(converted.value()));
            }
            obj.emplace("v", score::json::Any(std::move(list)));
            break;
        }
        case KvsValue::Type::kObject:
        {
            return KvsValueToAny(std::get<KvsValue::Object>(kvs_value.GetValue()));
        }
        default:
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
    }

    return score::json::Any(std::move(obj));
}

score::Result<score::json::Any> KvsValueToAny(const KvsValue::Object& object) noexcept
{
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
            const auto& array{std::get<KvsValue::Array>(value.GetValue())};
            return std::all_of(array.begin(), array.end(), [](const KvsValue& element) {
                return HasUniqueKeys(element);
            });
        }
        case KvsValue::Type::kObject:
        {
            const auto& object{std::get<KvsValue::Object>(value.GetValue())};
            std::unordered_set<std::string_view> keys;
            keys.reserve(object.size());
            for (const auto& [key, member] : object)
            {
                if (!keys.insert(key).second || !HasUniqueKeys(member))
                {
                    return false;
                }
            }
            return true;
        }
        default:
        {
            return true;
        }
    }
}

}  // namespace score::kvs

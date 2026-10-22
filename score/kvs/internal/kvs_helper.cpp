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

namespace score::mw::per::kvs
{

uint32_t ParseHashAdler32(std::istream& in) noexcept
{
    std::array<uint8_t, kHashSize> buf{};
    in.read(reinterpret_cast<char*>(buf.data()), buf.size());

    uint32_t value{(uint32_t(buf[0]) << 24) | (uint32_t(buf[1]) << 16) | (uint32_t(buf[2]) << 8) | uint32_t(buf[3])};
    return value;
}

// Optimized version: processes data in blocks to reduce modulo operations.
uint32_t CalculateHashAdler32(const std::string& data) noexcept
{
    constexpr size_t kAdler32Nmax{5552U};
    constexpr uint32_t kAdler32Base{65521U};
    uint32_t a{1U};
    uint32_t b{0U};
    size_t len{data.size()};
    size_t i{0U};

    // Process in blocks of 5552 bytes (as recommended for Adler-32).
    while (len > 0)
    {
        size_t tlen{len > kAdler32Nmax ? kAdler32Nmax : len};
        len -= tlen;
        for (size_t j{0U}; j < tlen; ++j, ++i)
        {
            a += static_cast<unsigned char>(data[i]);
            b += a;
        }
        a %= kAdler32Base;
        b %= kAdler32Base;
    }
    return (b << 16) | a;
}

std::array<uint8_t, kHashSize> GetHashBytesAdler32(uint32_t hash) noexcept
{
    std::array<uint8_t, kHashSize> value{uint8_t((hash >> 24) & 0xFF),
                                         uint8_t((hash >> 16) & 0xFF),
                                         uint8_t((hash >> 8) & 0xFF),
                                         uint8_t((hash) & 0xFF)};
    return value;
}

std::array<uint8_t, kHashSize> GetHashBytes(const std::string& data) noexcept
{
    uint32_t hash{CalculateHashAdler32(data)};
    std::array<uint8_t, kHashSize> value{GetHashBytesAdler32(hash)};
    return value;
}

bool CheckHash(const std::string& data_calculate, std::istream& data_parse) noexcept
{
    uint32_t calculated_hash{CalculateHashAdler32(data_calculate)};
    uint32_t parsed_hash{ParseHashAdler32(data_parse)};
    return calculated_hash == parsed_hash;
}

score::Result<KvsValue> AnyToKvsValue(const score::json::Any& any) noexcept
{
    auto o{any.As<score::json::Object>()};
    if (!o.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }

    const auto& obj_any{o.value().get()};
    auto type{obj_any.find("t")};
    auto value{obj_any.find("v")};
    if (type == obj_any.end() || value == obj_any.end())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }

    auto type_str{type->second.As<std::string>()};
    if (!type_str.has_value())
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }

    const std::string_view type_str_view{type_str.value().get()};
    const score::json::Any& value_any{value->second};

    if (type_str_view == "i32")
    {
        auto n{value_any.As<int32_t>()};
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(static_cast<int32_t>(n.value()));
    }
    else if (type_str_view == "u32")
    {
        auto n{value_any.As<uint32_t>()};
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(static_cast<uint32_t>(n.value()));
    }
    else if (type_str_view == "i64")
    {
        auto n{value_any.As<int64_t>()};
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(static_cast<int64_t>(n.value()));
    }
    else if (type_str_view == "u64")
    {
        auto n{value_any.As<uint64_t>()};
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(static_cast<uint64_t>(n.value()));
    }
    else if (type_str_view == "f64")
    {
        auto n{value_any.As<double>()};
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(n.value());
    }
    else if (type_str_view == "bool")
    {
        auto b{value_any.As<bool>()};
        if (!b.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(b.value());
    }
    else if (type_str_view == "str")
    {
        auto s{value_any.As<std::string>()};
        if (!s.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(s.value().get());
    }
    else if (type_str_view == "null")
    {
        if (!value_any.As<score::json::Null>().has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        return KvsValue(nullptr);
    }
    else if (type_str_view == "arr")
    {
        auto l{value_any.As<score::json::List>()};
        if (!l.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        KvsValue::Array arr;
        for (const auto& elem : l.value().get())
        {
            auto conv{AnyToKvsValue(elem)};
            if (!conv)
            {
                return score::MakeUnexpected(ErrorCode::kInvalidValueType);
            }
            arr.emplace_back(std::move(conv.value()));
        }
        return KvsValue(std::move(arr));
    }
    else if (type_str_view == "obj")
    {
        auto obj{value_any.As<score::json::Object>()};
        if (!obj.has_value())
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
        KvsValue::Object map;
        for (const auto& [key, val_any] : obj.value().get())
        {
            auto conv{AnyToKvsValue(val_any)};
            if (!conv)
            {
                return score::MakeUnexpected(ErrorCode::kInvalidValueType);
            }
            map.emplace_back(std::string(key.GetAsStringView()), std::move(conv.value()));
        }
        return KvsValue(std::move(map));
    }
    else
    {
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }
}

score::Result<score::json::Any> KvsValueToAny(const KvsValue& kv) noexcept
{
    score::json::Object obj;
    switch (kv.GetType())
    {
        case KvsValue::Type::kI32:
        {
            obj.emplace("t", score::json::Any(std::string("i32")));
            obj.emplace("v", score::json::Any(static_cast<int32_t>(std::get<int32_t>(kv.GetValue()))));
            break;
        }
        case KvsValue::Type::kU32:
        {
            obj.emplace("t", score::json::Any(std::string("u32")));
            obj.emplace("v", score::json::Any(static_cast<uint32_t>(std::get<uint32_t>(kv.GetValue()))));
            break;
        }
        case KvsValue::Type::kI64:
        {
            obj.emplace("t", score::json::Any(std::string("i64")));
            obj.emplace("v", score::json::Any(static_cast<int64_t>(std::get<int64_t>(kv.GetValue()))));
            break;
        }
        case KvsValue::Type::kU64:
        {
            obj.emplace("t", score::json::Any(std::string("u64")));
            obj.emplace("v", score::json::Any(static_cast<uint64_t>(std::get<uint64_t>(kv.GetValue()))));
            break;
        }
        case KvsValue::Type::kF64:
        {
            obj.emplace("t", score::json::Any(std::string("f64")));
            obj.emplace("v", score::json::Any(std::get<double>(kv.GetValue())));
            break;
        }
        case KvsValue::Type::kBoolean:
        {
            obj.emplace("t", score::json::Any(std::string("bool")));
            obj.emplace("v", score::json::Any(std::get<bool>(kv.GetValue())));
            break;
        }
        case KvsValue::Type::kString:
        {
            obj.emplace("t", score::json::Any(std::string("str")));
            obj.emplace("v", score::json::Any(std::get<std::string>(kv.GetValue())));
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
            for (const auto& elem : std::get<KvsValue::Array>(kv.GetValue()))
            {
                auto conv{KvsValueToAny(elem)};
                if (!conv)
                {
                    return score::MakeUnexpected(ErrorCode::kInvalidValueType);
                }
                list.push_back(std::move(conv.value()));
            }
            obj.emplace("v", score::json::Any(std::move(list)));
            break;
        }
        case KvsValue::Type::kObject:
        {
            obj.emplace("t", score::json::Any(std::string("obj")));
            score::json::Object inner_obj;
            for (const auto& [key, value] : std::get<KvsValue::Object>(kv.GetValue()))
            {
                auto conv{KvsValueToAny(value)};
                if (!conv)
                {
                    return score::MakeUnexpected(ErrorCode::kInvalidValueType);
                }
                inner_obj.emplace(key, std::move(conv.value()));
            }
            obj.emplace("v", score::json::Any(std::move(inner_obj)));
            break;
        }
        default:
        {
            return score::MakeUnexpected(ErrorCode::kInvalidValueType);
        }
    }

    return score::json::Any(std::move(obj));
}

}  // namespace score::mw::per::kvs

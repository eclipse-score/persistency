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

#include "kvs_helper.hpp"

namespace score::mw::per::kvs
{

// Optimized version: processes data in blocks to reduce modulo operations.
uint32_t calculate_hash_adler32(const std::string& data) noexcept
{
    constexpr size_t ADLER32_NMAX = 5552;
    constexpr uint32_t ADLER32_BASE = 65521;
    uint32_t a = 1;
    uint32_t b = 0;
    size_t len = data.size();
    size_t i = 0;

    // Process in blocks of 5552 bytes (as recommended for Adler-32).
    while (len > 0)
    {
        const size_t tlen = len > ADLER32_NMAX ? ADLER32_NMAX : len;
        len -= tlen;
        for (size_t j = 0; j < tlen; ++j, ++i)
        {
            a += static_cast<unsigned char>(data[i]);
            b += a;
        }
        a %= ADLER32_BASE;
        b %= ADLER32_BASE;
    }
    return (b << 16) | a;
}

uint32_t parse_hash_adler32(std::istream& in) noexcept
{
    std::array<uint8_t, 4> buf{};
    in.read(reinterpret_cast<char*>(buf.data()), buf.size());

    const uint32_t value = (static_cast<uint32_t>(buf[0]) << 24) | (static_cast<uint32_t>(buf[1]) << 16) |
                           (static_cast<uint32_t>(buf[2]) << 8) | static_cast<uint32_t>(buf[3]);
    return value;
}

std::array<uint8_t, 4> get_hash_bytes_adler32(const uint32_t hash) noexcept
{
    std::array<uint8_t, 4> value = {static_cast<uint8_t>((hash >> 24) & 0xFF),
                                    static_cast<uint8_t>((hash >> 16) & 0xFF),
                                    static_cast<uint8_t>((hash >> 8) & 0xFF),
                                    static_cast<uint8_t>((hash) & 0xFF)};
    return value;
}

std::array<uint8_t, 4> get_hash_bytes(const std::string& data) noexcept
{
    const uint32_t hash = calculate_hash_adler32(data);
    std::array<uint8_t, 4> value = get_hash_bytes_adler32(hash);
    return value;
}

bool check_hash(const std::string& data_calculate, std::istream& data_parse) noexcept
{
    const uint32_t calculated_hash = calculate_hash_adler32(data_calculate);
    const uint32_t parsed_hash = parse_hash_adler32(data_parse);
    return calculated_hash == parsed_hash;
}

score::Result<KvsValue> any_to_kvsvalue(const score::json::Any& any) noexcept
{
    const auto o = any.As<score::json::Object>();
    if (!o.has_value())
    {
        return score::MakeUnexpected(ErrorCode::InvalidValueType);
    }

    const auto& objAny = o.value().get();
    const auto type = objAny.find("t");
    const auto value = objAny.find("v");
    if (type == objAny.end() || value == objAny.end())
    {
        return score::MakeUnexpected(ErrorCode::InvalidValueType);
    }

    const auto typeStr = type->second.As<std::string>();
    if (!typeStr.has_value())
    {
        return score::MakeUnexpected(ErrorCode::InvalidValueType);
    }

    const std::string_view typeStrV = typeStr.value().get();
    const score::json::Any& valueAny = value->second;

    if (typeStrV == "i32")
    {
        const auto n = valueAny.As<int32_t>();
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        return KvsValue(static_cast<int32_t>(n.value()));
    }
    else if (typeStrV == "u32")
    {
        const auto n = valueAny.As<uint32_t>();
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        return KvsValue(static_cast<uint32_t>(n.value()));
    }
    else if (typeStrV == "i64")
    {
        const auto n = valueAny.As<int64_t>();
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        return KvsValue(static_cast<int64_t>(n.value()));
    }
    else if (typeStrV == "u64")
    {
        const auto n = valueAny.As<uint64_t>();
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        return KvsValue(static_cast<uint64_t>(n.value()));
    }
    else if (typeStrV == "f64")
    {
        const auto n = valueAny.As<double>();
        if (!n.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        return KvsValue(n.value());
    }
    else if (typeStrV == "bool")
    {
        const auto b = valueAny.As<bool>();
        if (!b.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        return KvsValue(b.value());
    }
    else if (typeStrV == "str")
    {
        const auto s = valueAny.As<std::string>();
        if (!s.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        return KvsValue(s.value().get());
    }
    else if (typeStrV == "null")
    {
        if (!valueAny.As<score::json::Null>().has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        return KvsValue(nullptr);
    }
    else if (typeStrV == "arr")
    {
        const auto l = valueAny.As<score::json::List>();
        if (!l.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        KvsValue::Array arr;
        for (const auto& elem : l.value().get())
        {
            auto conv = any_to_kvsvalue(elem);
            if (!conv)
            {
                return score::MakeUnexpected(ErrorCode::InvalidValueType);
            }
            arr.emplace_back(std::move(conv.value()));
        }
        return KvsValue(std::move(arr));
    }
    else if (typeStrV == "obj")
    {
        const auto obj = valueAny.As<score::json::Object>();
        if (!obj.has_value())
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
        KvsValue::Object map;
        for (const auto& [key, valAny] : obj.value().get())
        {
            auto conv = any_to_kvsvalue(valAny);
            if (!conv)
            {
                return score::MakeUnexpected(ErrorCode::InvalidValueType);
            }
            map.emplace_back(std::string(key.GetAsStringView()), std::move(conv.value()));
        }
        return KvsValue(std::move(map));
    }
    else
    {
        return score::MakeUnexpected(ErrorCode::InvalidValueType);
    }
}

score::Result<score::json::Any> kvsvalue_to_any(const KvsValue& kv) noexcept
{
    score::json::Object obj;
    switch (kv.getType())
    {
        case KvsValue::Type::i32:
        {
            obj.emplace("t", score::json::Any(std::string("i32")));
            obj.emplace("v", score::json::Any(static_cast<int32_t>(std::get<int32_t>(kv.getValue()))));
            break;
        }
        case KvsValue::Type::u32:
        {
            obj.emplace("t", score::json::Any(std::string("u32")));
            obj.emplace("v", score::json::Any(static_cast<uint32_t>(std::get<uint32_t>(kv.getValue()))));
            break;
        }
        case KvsValue::Type::i64:
        {
            obj.emplace("t", score::json::Any(std::string("i64")));
            obj.emplace("v", score::json::Any(static_cast<int64_t>(std::get<int64_t>(kv.getValue()))));
            break;
        }
        case KvsValue::Type::u64:
        {
            obj.emplace("t", score::json::Any(std::string("u64")));
            obj.emplace("v", score::json::Any(static_cast<uint64_t>(std::get<uint64_t>(kv.getValue()))));
            break;
        }
        case KvsValue::Type::f64:
        {
            obj.emplace("t", score::json::Any(std::string("f64")));
            obj.emplace("v", score::json::Any(std::get<double>(kv.getValue())));
            break;
        }
        case KvsValue::Type::Boolean:
        {
            obj.emplace("t", score::json::Any(std::string("bool")));
            obj.emplace("v", score::json::Any(std::get<bool>(kv.getValue())));
            break;
        }
        case KvsValue::Type::String:
        {
            obj.emplace("t", score::json::Any(std::string("str")));
            obj.emplace("v", score::json::Any(std::get<std::string>(kv.getValue())));
            break;
        }
        case KvsValue::Type::Null:
        {
            obj.emplace("t", score::json::Any(std::string("null")));
            obj.emplace("v", score::json::Any(score::json::Null{}));
            break;
        }
        case KvsValue::Type::Array:
        {
            obj.emplace("t", score::json::Any(std::string("arr")));
            score::json::List list;
            for (const auto& elem : std::get<KvsValue::Array>(kv.getValue()))
            {
                auto conv = kvsvalue_to_any(elem);
                if (!conv)
                {
                    return score::MakeUnexpected(ErrorCode::InvalidValueType);
                }
                list.push_back(std::move(conv.value()));
            }
            obj.emplace("v", score::json::Any(std::move(list)));
            break;
        }
        case KvsValue::Type::Object:
        {
            obj.emplace("t", score::json::Any(std::string("obj")));
            score::json::Object inner_obj;
            for (const auto& [key, value] : std::get<KvsValue::Object>(kv.getValue()))
            {
                auto conv = kvsvalue_to_any(value);
                if (!conv)
                {
                    return score::MakeUnexpected(ErrorCode::InvalidValueType);
                }
                inner_obj.emplace(key, std::move(conv.value()));
            }
            obj.emplace("v", score::json::Any(std::move(inner_obj)));
            break;
        }
        default:
        {
            return score::MakeUnexpected(ErrorCode::InvalidValueType);
        }
    }

    return score::json::Any(std::move(obj));
}

}  // namespace score::mw::per::kvs

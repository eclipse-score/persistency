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

#include "score/kvs/kvs.h"
#include "score/kvs/kvs_builder.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace
{

using score::kvs::Kvs;
using score::kvs::KvsValue;

/// Print an error and abort if the value of `key` is not as expected.
void ExpectValue(const bool is_expected, const std::string_view key)
{
    if (!is_expected)
    {
        std::cerr << "unexpected value of \"" << key << "\"" << '\n';
        std::abort();
    }
}

/// Get the value of `key`, abort if it's missing.
KvsValue GetOrAbort(const Kvs& kvs, const std::string_view key)
{
    auto result{kvs.GetValue(key)};
    if (!result.has_value())
    {
        std::cerr << "failed to get \"" << key << "\"" << '\n';
        std::abort();
    }
    return std::move(*result);
}

/// Set the value of `key`, abort on failure.
void SetOrAbort(Kvs& kvs, const std::string_view key, const KvsValue& value)
{
    if (!kvs.SetValue(key, value).has_value())
    {
        std::cerr << "failed to set \"" << key << "\"" << '\n';
        std::abort();
    }
}

/// Get the object member named `key`, abort if it's missing.
const KvsValue& MemberOrAbort(const KvsValue::Object& object, const std::string_view key)
{
    for (const auto& member : object)
    {
        if (member.first == key)
        {
            return member.second;
        }
    }
    std::cerr << "missing object member \"" << key << "\"" << '\n';
    std::abort();
}

bool IsU8(const KvsValue& value, const std::uint8_t expected)
{
    const auto* const actual{std::get_if<std::uint8_t>(&value.GetValue())};
    return (actual != nullptr) && (*actual == expected);
}

bool IsU16(const KvsValue& value, const std::uint16_t expected)
{
    const auto* const actual{std::get_if<std::uint16_t>(&value.GetValue())};
    return (actual != nullptr) && (*actual == expected);
}

bool IsI8(const KvsValue& value, const std::int8_t expected)
{
    const auto* const actual{std::get_if<std::int8_t>(&value.GetValue())};
    return (actual != nullptr) && (*actual == expected);
}

bool IsI16(const KvsValue& value, const std::int16_t expected)
{
    const auto* const actual{std::get_if<std::int16_t>(&value.GetValue())};
    return (actual != nullptr) && (*actual == expected);
}

bool IsF32(const KvsValue& value, const float expected)
{
    const auto* const actual{std::get_if<float>(&value.GetValue())};
    return (actual != nullptr) && (*actual == expected);
}

bool IsF64(const KvsValue& value, const double expected)
{
    const auto* const actual{std::get_if<double>(&value.GetValue())};
    return (actual != nullptr) && (*actual == expected);
}

bool IsBool(const KvsValue& value, const bool expected)
{
    const auto* const actual{std::get_if<bool>(&value.GetValue())};
    return (actual != nullptr) && (*actual == expected);
}

bool IsString(const KvsValue& value, const std::string_view expected)
{
    const auto* const actual{std::get_if<std::string>(&value.GetValue())};
    return (actual != nullptr) && (*actual == expected);
}

bool IsNull(const KvsValue& value)
{
    return value.GetType() == KvsValue::Type::kNull;
}

/// Check for the `[number, bool, string]` array that `createtestdata` writes.
bool IsTestArray(const KvsValue& value, const double number, const bool boolean, const std::string_view string)
{
    const auto* const array{std::get_if<KvsValue::Array>(&value.GetValue())};
    return (array != nullptr) && (array->size() == 3U) && IsF64((*array)[0U], number) &&
           IsBool((*array)[1U], boolean) && IsString((*array)[2U], string);
}

/// Execute scenario for `store` test.
void StoreScenario(const std::string& working_dir)
{
    using namespace score::kvs;

    const InstanceId instance_id{0};
    auto build_result{KvsBuilder{instance_id}
                          .Defaults(KvsDefaults::kIgnored)
                          .KvsLoad(KvsLoad::kRequired)
                          .WorkingDir(working_dir)
                          .Build()};
    if (!build_result.has_value())
    {
        std::cerr << "failed to build KVS" << '\n';
        std::abort();
    }
    auto kvs{std::move(*build_result)};

    ExpectValue(IsU8(GetOrAbort(kvs, "u8"), std::uint8_t{123}), "u8");
    ExpectValue(IsU16(GetOrAbort(kvs, "u16"), std::uint16_t{12345}), "u16");
    ExpectValue(IsI8(GetOrAbort(kvs, "i8"), std::int8_t{-123}), "i8");
    ExpectValue(IsI16(GetOrAbort(kvs, "i16"), std::int16_t{-12345}), "i16");
    ExpectValue(IsF32(GetOrAbort(kvs, "f32"), 12.5F), "f32");
    ExpectValue(IsF64(GetOrAbort(kvs, "number"), 123.0), "number");
    ExpectValue(IsBool(GetOrAbort(kvs, "bool"), true), "bool");
    ExpectValue(IsString(GetOrAbort(kvs, "string"), "First"), "string");
    ExpectValue(IsNull(GetOrAbort(kvs, "null")), "null");
    ExpectValue(IsTestArray(GetOrAbort(kvs, "array"), 456.0, false, "Second"), "array");

    // Object members are looked up by key, so the check doesn't depend on their order.
    const auto object_value{GetOrAbort(kvs, "object")};
    const auto* const object{std::get_if<KvsValue::Object>(&object_value.GetValue())};
    ExpectValue(object != nullptr, "object");
    ExpectValue(object->size() == 5U, "object");
    ExpectValue(IsF64(MemberOrAbort(*object, "sub-number"), 789.0), "object.sub-number");
    ExpectValue(IsBool(MemberOrAbort(*object, "sub-bool"), true), "object.sub-bool");
    ExpectValue(IsString(MemberOrAbort(*object, "sub-string"), "Third"), "object.sub-string");
    ExpectValue(IsNull(MemberOrAbort(*object, "sub-null")), "object.sub-null");
    ExpectValue(IsTestArray(MemberOrAbort(*object, "sub-array"), 1246.0, false, "Fourth"), "object.sub-array");

    std::cout << "PASSED" << '\n';
}

/// Execute scenario for `load` test.
void LoadScenario(const std::string& working_dir)
{
    using namespace score::kvs;

    const InstanceId instance_id{0};
    auto build_result{KvsBuilder{instance_id}
                          .Defaults(KvsDefaults::kIgnored)
                          .KvsLoad(KvsLoad::kIgnored)
                          .WorkingDir(working_dir)
                          .Build()};
    if (!build_result.has_value())
    {
        std::cerr << "failed to build KVS" << '\n';
        std::abort();
    }
    auto kvs{std::move(*build_result)};

    SetOrAbort(kvs, "i8", KvsValue{std::int8_t{-123}});
    SetOrAbort(kvs, "u8", KvsValue{std::uint8_t{123}});
    SetOrAbort(kvs, "i16", KvsValue{std::int16_t{-12345}});
    SetOrAbort(kvs, "u16", KvsValue{std::uint16_t{12345}});
    SetOrAbort(kvs, "f32", KvsValue{12.5F});
    SetOrAbort(kvs, "i32", KvsValue{static_cast<int32_t>(-123)});
    SetOrAbort(kvs, "u32", KvsValue{static_cast<uint32_t>(123)});
    SetOrAbort(kvs, "i64", KvsValue{static_cast<int64_t>(-12345)});
    SetOrAbort(kvs, "u64", KvsValue{static_cast<uint64_t>(12345)});
    SetOrAbort(kvs, "f64", KvsValue{12345.6});
    SetOrAbort(kvs, "bool", KvsValue{true});
    SetOrAbort(kvs, "string", KvsValue{std::string{"string-value"}});
    SetOrAbort(kvs, "null", KvsValue{nullptr});
    SetOrAbort(kvs,
               "array",
               KvsValue{std::vector<KvsValue>{
                   {KvsValue{static_cast<int64_t>(555)}, KvsValue{std::string{"string-in-array"}}}}});
    const KvsValue::Object obj{{std::string{"f64-in-object"}, KvsValue{123.456}}};
    SetOrAbort(kvs, "object", KvsValue{obj});

    if (!kvs.Flush().has_value())
    {
        std::cerr << "failed to flush KVS" << '\n';
        std::abort();
    }
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "invalid number of arguments" << '\n';
        std::abort();
    }
    const std::string test_scenario_name{argv[1]};
    const std::string working_dir{argv[2]};

    if (test_scenario_name == "store")
    {
        StoreScenario(working_dir);
    }
    else if (test_scenario_name == "load")
    {
        LoadScenario(working_dir);
    }
    else
    {
        std::cerr << "invalid test scenario name" << '\n';
        std::abort();
    }
}

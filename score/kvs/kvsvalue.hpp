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
#ifndef SCORE_LIB_KVS_KVSVALUE_HPP
#define SCORE_LIB_KVS_KVSVALUE_HPP

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace score::mw::per::kvs
{

/// @brief Value that holds a number, boolean, string, null, array or object.
///
/// The KvsValue class provides a type-safe way to store and retrieve values of different types.
/// It uses a std::variant to hold the underlying value and an enum to track the type of the value.
/// Access the underlying value using getValue() and `std::get`.
///
/// Example:
/// @code
/// KvsValue numberValue(42.0);
/// KvsValue stringValue("Hello, World!");
/// KvsValue arrayValue(KvsValue::Array{numberValue, stringValue});
/// KvsValue objectValue(KvsValue::Object{{"number", numberValue}, {"text", stringValue}});
///
/// if (numberValue.getType() == KvsValue::Type::f64) {
///     double number = std::get<double>(numberValue.getValue());
/// }
/// @endcode
class KvsValue final
{
  public:
    /// @brief Array of values.
    ///
    /// KvsValue is incomplete here.
    /// Both containers support incomplete element types.
    using Array = std::vector<KvsValue>;

    /// @brief Object as an ordered list of key-value pairs.
    using Object = std::vector<std::pair<std::string, KvsValue>>;

    /// @brief Type of the held value.
    enum class Type
    {
        /// `int32_t`
        i32,
        /// `uint32_t`
        u32,
        /// `int64_t`
        i64,
        /// `uint64_t`
        u64,
        /// `double`
        f64,
        /// `bool`
        Boolean,
        /// `std::string`
        String,
        /// `std::nullptr_t`
        Null,
        /// KvsValue::Array
        Array,
        /// KvsValue::Object
        Object
    };

    using ValueVariant =
        std::variant<int32_t, uint32_t, int64_t, uint64_t, double, bool, std::string, std::nullptr_t, Array, Object>;

    /// @brief The Type enumerators must match the order of the variant alternatives.
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::i32), ValueVariant>, int32_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::u32), ValueVariant>, uint32_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::i64), ValueVariant>, int64_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::u64), ValueVariant>, uint64_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::f64), ValueVariant>, double>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Boolean), ValueVariant>, bool>);
    static_assert(
        std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::String), ValueVariant>, std::string>);
    static_assert(
        std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Null), ValueVariant>, std::nullptr_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Array), ValueVariant>, Array>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::Object), ValueVariant>, Object>);
    static_assert(std::variant_size_v<ValueVariant> == static_cast<size_t>(Type::Object) + 1U);

    /// @brief Constructs a value of type Type::i32.
    explicit KvsValue(int32_t number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::u32.
    explicit KvsValue(uint32_t number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::i64.
    explicit KvsValue(int64_t number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::u64.
    explicit KvsValue(uint64_t number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::f64.
    explicit KvsValue(double number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::Boolean.
    explicit KvsValue(bool boolean) noexcept : value_(boolean) {}
    /// @brief Constructs a value of type Type::String from a C string.
    explicit KvsValue(const char* str) noexcept;
    /// @brief Constructs a value of type Type::String.
    explicit KvsValue(const std::string& str) noexcept : value_(str) {}
    /// @brief Constructs a value of type Type::Null.
    explicit KvsValue(std::nullptr_t) noexcept : value_(nullptr) {}
    /// @brief Constructs a value of type Type::Array by copying the elements.
    explicit KvsValue(const Array& array) noexcept;

    /// @brief Constructs a value of type Type::Array by moving the elements.
    explicit KvsValue(Array&& array) noexcept;

    /// @brief Constructs a value of type Type::Object by copying the members.
    explicit KvsValue(const Object& object) noexcept;

    /// @brief Constructs a value of type Type::Object by moving the members.
    explicit KvsValue(Object&& object) noexcept;

    /// @brief Constructs a value of type Type::Object from a map.
    explicit KvsValue(const std::unordered_map<std::string, KvsValue>& object) noexcept;

    /// @brief Copy constructor.
    KvsValue(const KvsValue& other) noexcept;

    /// @brief Copy assignment operator.
    KvsValue& operator=(const KvsValue& other) noexcept;

    /// @brief Move constructor.
    KvsValue(KvsValue&& other) noexcept;

    /// @brief Move assignment operator.
    KvsValue& operator=(KvsValue&& other) noexcept;

    /// @brief Destructor.
    ~KvsValue() noexcept;

    /// @brief Gets the type of the value.
    /// @return The type of the held value.
    Type getType() const noexcept
    {
        return static_cast<Type>(value_.index());
    }

    /// @brief Accesses the underlying value.
    ///
    /// Use `std::get` to retrieve the value.
    ///
    /// @return The underlying variant.
    const ValueVariant& getValue() const noexcept
    {
        return value_;
    }

  private:
    /// The underlying value.
    ValueVariant value_;
};

/// Key-value storage map type.
using KvsMap = std::unordered_map<std::string, KvsValue>;

}  // namespace score::mw::per::kvs

#endif  // SCORE_LIB_KVS_KVSVALUE_HPP

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
#ifndef SCORE_KVS_KVS_VALUE_H
#define SCORE_KVS_KVS_VALUE_H

#include <cstddef>
#include <cstdint>
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
/// Access the underlying value using GetValue() and `std::get`.
///
/// Example:
/// @code
/// KvsValue number_value(42.0);
/// KvsValue string_value("Hello, World!");
/// KvsValue array_value(KvsValue::Array{number_value, string_value});
/// KvsValue object_value(KvsValue::Object{{"number", number_value}, {"text", string_value}});
///
/// if (number_value.GetType() == KvsValue::Type::kF64) {
///     double number = std::get<double>(number_value.GetValue());
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
        kI32,
        /// `uint32_t`
        kU32,
        /// `int64_t`
        kI64,
        /// `uint64_t`
        kU64,
        /// `double`
        kF64,
        /// `bool`
        kBoolean,
        /// `std::string`
        kString,
        /// `std::nullptr_t`
        kNull,
        /// KvsValue::Array
        kArray,
        /// KvsValue::Object
        kObject,

        // Deprecated enumerators.
        // Each one is an alias of its k-prefixed replacement and is removed in a future release.
        i32 [[deprecated("Use kI32 instead. The snake_case API is removed in a future release.")]] = kI32,
        u32 [[deprecated("Use kU32 instead. The snake_case API is removed in a future release.")]] = kU32,
        i64 [[deprecated("Use kI64 instead. The snake_case API is removed in a future release.")]] = kI64,
        u64 [[deprecated("Use kU64 instead. The snake_case API is removed in a future release.")]] = kU64,
        f64 [[deprecated("Use kF64 instead. The snake_case API is removed in a future release.")]] = kF64,
        Boolean [[deprecated("Use kBoolean instead. The snake_case API is removed in a future release.")]] = kBoolean,
        String [[deprecated("Use kString instead. The snake_case API is removed in a future release.")]] = kString,
        Null [[deprecated("Use kNull instead. The snake_case API is removed in a future release.")]] = kNull,
        Array [[deprecated("Use kArray instead. The snake_case API is removed in a future release.")]] = kArray,
        Object [[deprecated("Use kObject instead. The snake_case API is removed in a future release.")]] = kObject,
    };

    using ValueVariant =
        std::variant<int32_t, uint32_t, int64_t, uint64_t, double, bool, std::string, std::nullptr_t, Array, Object>;

    /// @brief The Type enumerators must match the order of the variant alternatives.
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kI32), ValueVariant>, int32_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kU32), ValueVariant>, uint32_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kI64), ValueVariant>, int64_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kU64), ValueVariant>, uint64_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kF64), ValueVariant>, double>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kBoolean), ValueVariant>, bool>);
    static_assert(
        std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kString), ValueVariant>, std::string>);
    static_assert(
        std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kNull), ValueVariant>, std::nullptr_t>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kArray), ValueVariant>, Array>);
    static_assert(std::is_same_v<std::variant_alternative_t<static_cast<size_t>(Type::kObject), ValueVariant>, Object>);
    static_assert(std::variant_size_v<ValueVariant> == static_cast<size_t>(Type::kObject) + 1U);

    /// @brief Constructs a value of type Type::kI32.
    explicit KvsValue(int32_t number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::kU32.
    explicit KvsValue(uint32_t number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::kI64.
    explicit KvsValue(int64_t number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::kU64.
    explicit KvsValue(uint64_t number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::kF64.
    explicit KvsValue(double number) noexcept : value_(number) {}
    /// @brief Constructs a value of type Type::kBoolean.
    explicit KvsValue(bool boolean) noexcept : value_(boolean) {}
    /// @brief Constructs a value of type Type::kString from a C string.
    explicit KvsValue(const char* str) noexcept;
    /// @brief Constructs a value of type Type::kString.
    explicit KvsValue(const std::string& str) noexcept : value_(str) {}
    /// @brief Constructs a value of type Type::kNull.
    explicit KvsValue(std::nullptr_t) noexcept : value_(nullptr) {}
    /// @brief Constructs a value of type Type::kArray by copying the elements.
    explicit KvsValue(const Array& array) noexcept;

    /// @brief Constructs a value of type Type::kArray by moving the elements.
    explicit KvsValue(Array&& array) noexcept;

    /// @brief Constructs a value of type Type::kObject by copying the members.
    explicit KvsValue(const Object& object) noexcept;

    /// @brief Constructs a value of type Type::kObject by moving the members.
    explicit KvsValue(Object&& object) noexcept;

    /// @brief Constructs a value of type Type::kObject from a map.
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
    Type GetType() const noexcept
    {
        return static_cast<Type>(value_.index());
    }

    /// @brief Accesses the underlying value.
    ///
    /// Use `std::get` to retrieve the value.
    ///
    /// @return The underlying variant.
    const ValueVariant& GetValue() const noexcept
    {
        return value_;
    }

    // Deprecated snake_case API.
    // Each entry forwards to its PascalCase replacement and is removed in a future release.

    /// @deprecated Use `GetType()` instead.
    [[deprecated("Use GetType() instead. The snake_case API is removed in a future release.")]]
    Type getType() const noexcept
    {
        return GetType();
    }

    /// @deprecated Use `GetValue()` instead.
    [[deprecated("Use GetValue() instead. The snake_case API is removed in a future release.")]]
    const ValueVariant& getValue() const noexcept
    {
        return GetValue();
    }

  private:
    /// The underlying value.
    ValueVariant value_;
};

}  // namespace score::mw::per::kvs

#endif  // SCORE_KVS_KVS_VALUE_H

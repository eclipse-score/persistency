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

#include "kvsvalue.hpp"
#include "score/assert.hpp"

namespace score::mw::per::kvs
{

KvsValue::KvsValue(const char* str) noexcept : value_(nullptr), type_(Type::String)
{
    SCORE_LANGUAGE_FUTURECPP_PRECONDITION_PRD_MESSAGE(str != nullptr, "KvsValue: str must not be null");
    value_ = std::string(str);
}

KvsValue::KvsValue(const Array& array) noexcept : value_(array), type_(Type::Array) {}

KvsValue::KvsValue(Array&& array) noexcept : value_(std::move(array)), type_(Type::Array) {}

KvsValue::KvsValue(const Object& object) noexcept : value_(object), type_(Type::Object) {}

KvsValue::KvsValue(Object&& object) noexcept : value_(std::move(object)), type_(Type::Object) {}

KvsValue::KvsValue(const std::unordered_map<std::string, KvsValue>& object) noexcept : type_(Type::Object)
{
    Object entries;
    entries.reserve(object.size());
    for (const auto& [key, item] : object)
    {
        entries.emplace_back(key, item);
    }
    value_ = std::move(entries);
}

/* copy constructor */
KvsValue::KvsValue(const KvsValue& other) noexcept : value_(other.value_), type_(other.type_) {}

/* move constructor */
KvsValue::KvsValue(KvsValue&& other) noexcept : value_(std::move(other.value_)), type_(other.type_) {}

/* copy Assignment Operator */
KvsValue& KvsValue::operator=(const KvsValue& other) noexcept
{
    if (this != &other)
    {
        value_ = other.value_;
        type_ = other.type_;
    }
    return *this;
}

/* move Assignment Operator */
KvsValue& KvsValue::operator=(KvsValue&& other) noexcept
{
    if (this != &other)
    {
        value_ = std::move(other.value_);
        type_ = other.type_;
    }
    return *this;
}

KvsValue::~KvsValue() noexcept = default;

} /* end namespace score::mw::per::kvs */

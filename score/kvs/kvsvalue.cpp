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

namespace score::mw::per::kvs
{

KvsValue::KvsValue(const Array& array) : value(array), type(Type::Array) {}

KvsValue::KvsValue(Array&& array) : value(std::move(array)), type(Type::Array) {}

KvsValue::KvsValue(const Object& object) : value(object), type(Type::Object) {}

KvsValue::KvsValue(Object&& object) : value(std::move(object)), type(Type::Object) {}

KvsValue::KvsValue(const std::unordered_map<std::string, KvsValue>& object) : type(Type::Object)
{
    Object entries;
    entries.reserve(object.size());
    for (const auto& [key, item] : object)
    {
        entries.emplace_back(key, item);
    }
    value = std::move(entries);
}

/* copy constructor */
KvsValue::KvsValue(const KvsValue& other) : value(other.value), type(other.type) {}

/* move constructor */
KvsValue::KvsValue(KvsValue&& other) noexcept : value(std::move(other.value)), type(other.type) {}

/* copy Assignment Operator */
KvsValue& KvsValue::operator=(const KvsValue& other)
{
    if (this != &other)
    {
        value = other.value;
        type = other.type;
    }
    return *this;
}

/* move Assignment Operator */
KvsValue& KvsValue::operator=(KvsValue&& other) noexcept
{
    if (this != &other)
    {
        value = std::move(other.value);
        type = other.type;
    }
    return *this;
}

KvsValue::~KvsValue() = default;

} /* end namespace score::mw::per::kvs */

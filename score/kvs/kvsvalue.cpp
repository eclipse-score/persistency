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

KvsValue::KvsValue(const Array& array) : value_(array), type_(Type::Array) {}

KvsValue::KvsValue(Array&& array) : value_(std::move(array)), type_(Type::Array) {}

KvsValue::KvsValue(const Object& object) : value_(object), type_(Type::Object) {}

KvsValue::KvsValue(Object&& object) : value_(std::move(object)), type_(Type::Object) {}

/* copy constructor */
KvsValue::KvsValue(const KvsValue& other) : value_(other.value_), type_(other.type_) {}

/* move constructor */
KvsValue::KvsValue(KvsValue&& other) noexcept : value_(std::move(other.value_)), type_(other.type_) {}

/* copy Assignment Operator */
KvsValue& KvsValue::operator=(const KvsValue& other)
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

KvsValue::~KvsValue() = default;

} /* end namespace score::mw::per::kvs */

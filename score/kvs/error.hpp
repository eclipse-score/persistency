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
#ifndef SCORE_LIB_KVS_INTERNAL_ERROR_HPP
#define SCORE_LIB_KVS_INTERNAL_ERROR_HPP

#include "score/kvs/error.h"

namespace score::mw::per::kvs::deprecated_headers
{

/// @deprecated Include `score/kvs/error.h` instead.
using ErrorHpp [[deprecated("score/kvs/error.hpp is deprecated. Include score/kvs/error.h instead.")]] = int;

// Referencing the alias emits the deprecation warning in every translation unit that includes this header.
static_assert(sizeof(ErrorHpp) > 0, "");

}  // namespace score::mw::per::kvs::deprecated_headers

#endif  // SCORE_LIB_KVS_INTERNAL_ERROR_HPP

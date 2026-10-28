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

#include "helpers/kvs_parameters.h"

using namespace score::json;

namespace
{

/// @brief Deserializes a load parameter: "defaults" or "kvs_load".
template <
    typename T,
    typename std::enable_if_t<std::is_same_v<T, score::kvs::KvsDefaults> || std::is_same_v<T, score::kvs::KvsLoad>,
                              bool> = true>
std::optional<T> DeserializeLoadParam(const score::json::Object& obj_root, const std::string& field_name)
{
    if (obj_root.find(field_name) != obj_root.end())
    {
        auto value_str{obj_root.at(field_name).As<std::string>().value().get()};
        if (value_str == "ignored")
        {
            return T::kIgnored;
        }
        else if (value_str == "optional")
        {
            return T::kOptional;
        }
        else if (value_str == "required")
        {
            return T::kRequired;
        }
        else
        {
            throw std::runtime_error{"Unknown load parameter"};
        }
    }

    return {};
}

}  // namespace

KvsParameters KvsParameters::FromJson(const std::string& json_str)
{
    // Load and parse provided JSON string.
    JsonParser parser;
    auto any_res{parser.FromBuffer(json_str)};
    if (!any_res)
    {
        throw score::result::Error(any_res.error());
    }

    return KvsParameters::FromValue(any_res.value().As<Object>().value().get());
}

KvsParameters KvsParameters::FromValue(const Object& object)
{
    // Take "kvs_parameters" field containing object.
    const auto& map_root{object.at("kvs_parameters")};
    const auto& obj_root{map_root.As<Object>().value().get()};

    KvsParameters params{obj_root.at("instance_id").As<uint64_t>().value()};

    // Deserialize "defaults".
    params.defaults = DeserializeLoadParam<score::kvs::KvsDefaults>(obj_root, "defaults");

    // Deserialize "kvs_load".
    params.kvs_load = DeserializeLoadParam<score::kvs::KvsLoad>(obj_root, "kvs_load");

    // Deserialize "dir".
    if (obj_root.find("dir") != obj_root.end())
    {
        params.dir = obj_root.at("dir").As<std::string>().value();
    }

    // Deserialize "snapshot_max_count".
    if (obj_root.find("snapshot_max_count") != obj_root.end())
    {
        params.snapshot_max_count = obj_root.at("snapshot_max_count").As<size_t>().value();
    }

    return params;
}

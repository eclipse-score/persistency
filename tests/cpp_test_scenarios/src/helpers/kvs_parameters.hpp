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
#pragma once

#include <kvs.hpp>

// Custom exception type for error code propagation (reusable in all scenario)
class ScenarioError : public std::runtime_error {
  public:
    score::mw::per::kvs::ErrorCode code;
    ScenarioError(score::mw::per::kvs::ErrorCode code, const std::string &msg)
        : std::runtime_error(msg), code(code) {}
};

namespace {

struct KvsParameters {
    uint64_t instance_id;
    std::optional<bool> need_defaults;
    std::optional<bool> need_kvs;
    std::optional<std::string> dir;
};

KvsParameters map_to_params(const std::string &data) {
    using namespace score::json;

    JsonParser parser;
    auto any_res{parser.FromBuffer(data)};
    if (!any_res) {
        throw ScenarioError(score::mw::per::kvs::ErrorCode::JsonParserError,
                            "Failed to parse JSON data");
    }
    const auto &map_root{
        any_res.value().As<Object>().value().get().at("kvs_parameters")};
    const auto &obj_root{map_root.As<Object>().value().get()};

    KvsParameters params;
    params.instance_id = obj_root.at("instance_id").As<double>().value();
    // Precedence: direct 'need_defaults' field overrides inference from
    // 'defaults'.
    if (obj_root.find("need_defaults") != obj_root.end()) {
        params.need_defaults = obj_root.at("need_defaults").As<bool>().value();
    } else {
        // If 'need_defaults' is not present, infer from 'defaults' field.
        if (obj_root.find("defaults") != obj_root.end()) {
            auto defaults_val =
                obj_root.at("defaults").As<std::string>().value();
            if (defaults_val.get() == "required") {
                params.need_defaults = true;
            } else if (defaults_val.get() == "optional" ||
                       defaults_val.get() == "without") {
                params.need_defaults = false;
            }
        }
    }
    if (obj_root.find("need_kvs") != obj_root.end()) {
        params.need_kvs = obj_root.at("need_kvs").As<bool>().value();
    }
    if (obj_root.find("dir") != obj_root.end()) {
        params.dir = obj_root.at("dir").As<std::string>().value();
    }

    // Explicitly check for missing defaults file if required
    if (params.need_defaults.value_or(false)) {
        if (params.dir.has_value()) {
            std::string defaults_path = *params.dir + "/kvs_" +
                                        std::to_string(params.instance_id) +
                                        "_default.json";
            std::ifstream defaults_file(defaults_path);
            if (!defaults_file.good()) {
                throw ScenarioError(
                    score::mw::per::kvs::ErrorCode::KvsFileReadError,
                    "Defaults file missing: " + defaults_path);
            }
        }
    }

    return params;
}

} // namespace
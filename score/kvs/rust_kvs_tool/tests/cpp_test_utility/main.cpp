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

#include "score/kvs/kvs.hpp"
#include "score/kvs/kvsbuilder.hpp"
#include <cassert>
#include <cstddef>
#include <iostream>

/// Execute scenario for `store` test.
void store_scenario(std::string working_dir)
{
    using namespace score::mw::per::kvs;

    // Initialize KVS.
    InstanceId instance_id{0};
    auto build_result{KvsBuilder{instance_id}
                          .need_defaults_flag(OpenNeedDefaults::Ignored)
                          .need_kvs_flag(OpenNeedKvs::Required)
                          .dir(std::move(working_dir))
                          .build()};
    if (!build_result.has_value())
    {
        std::cerr << "failed to build KVS" << std::endl;
        std::abort();
    }
    auto kvs{std::move(*build_result)};

    // Check structure is as expected.
    assert(std::get<double>(kvs.get_value("number")->getValue()) == 123.0f);

    std::cout << "PASSED" << std::endl;
}

/// Execute scenario for `load` test.
void load_scenario(std::string working_dir)
{
    using namespace score::mw::per::kvs;

    // Initialize KVS.
    InstanceId instance_id{0};
    auto build_result{KvsBuilder{instance_id}
                          .need_defaults_flag(OpenNeedDefaults::Ignored)
                          .need_kvs_flag(OpenNeedKvs::Ignored)
                          .dir(std::move(working_dir))
                          .build()};
    if (!build_result.has_value())
    {
        std::cerr << "failed to build KVS" << std::endl;
        std::abort();
    }
    auto kvs{std::move(*build_result)};

    // Set structure.
    score::cpp::ignore = kvs.set_value("i32", KvsValue{static_cast<int32_t>(-123)});
    score::cpp::ignore = kvs.set_value("u32", KvsValue{static_cast<uint32_t>(123)});
    score::cpp::ignore = kvs.set_value("i64", KvsValue{static_cast<int64_t>(-12345)});
    score::cpp::ignore = kvs.set_value("u64", KvsValue{static_cast<uint64_t>(12345)});
    score::cpp::ignore = kvs.set_value("f64", KvsValue{static_cast<double>(12345.6)});
    score::cpp::ignore = kvs.set_value("bool", KvsValue{static_cast<bool>(true)});
    score::cpp::ignore = kvs.set_value("string", KvsValue{std::string{"string-value"}});
    score::cpp::ignore = kvs.set_value("null", KvsValue{nullptr});
    score::cpp::ignore = kvs.set_value("array",
                                       KvsValue{std::vector<KvsValue>{{KvsValue{static_cast<int64_t>(555)},
                                                                       KvsValue{std::string{"string-in-array"}}}}});
    KvsValue::Object obj{{std::string{"f64-in-object"}, KvsValue{static_cast<double>(123.456)}}};
    score::cpp::ignore = kvs.set_value("object", KvsValue{obj});

    // Flush.
    if (!kvs.flush())
    {
        std::cerr << "failed to flush KVS" << std::endl;
        std::abort();
    }
}

int main(int argc, char** argv)
{
    // Get and process arguments.
    if (argc < 3)
    {
        std::cerr << "invalid number of arguments" << std::endl;
        std::abort();
    }
    std::string test_scenario_name{argv[1]};
    std::string working_dir{argv[2]};

    // Run selected scenario.
    if (test_scenario_name == "store")
    {
        store_scenario(working_dir);
    }
    else if (test_scenario_name == "load")
    {
        load_scenario(working_dir);
    }
    else
    {
        std::cerr << "invalid test scenario name" << std::endl;
        std::abort();
    }
}

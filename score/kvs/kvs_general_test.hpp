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

////////////////////////////////////////////////////////////////////////////////
/* The test_kvs_general files provide configuration data and methods needed for KVS tests */
////////////////////////////////////////////////////////////////////////////////

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

/* Change Private Members and final to public to allow access to member variables (kvs and
 * kvsbuilder) and derive from kvsvalue in unittests*/
#define private public
#define final
#include "score/kvs/kvsbuilder.hpp"
#undef private
#undef final
#include "score/kvs/internal/kvs_helper.hpp"
#include "score/filesystem/filesystem_mock.h"
#include "score/json/i_json_parser_mock.h"
#include "score/json/i_json_writer_mock.h"
using namespace score::mw::per::kvs;

////////////////////////////////////////////////////////////////////////////////

uint32_t adler32(const std::string& data);
void prepare_environment();
void cleanup_environment();
void cleanup_environment(const std::string& dir);
void store(const std::string& path, const std::string& hashPath, const std::string& data);

inline void prepare_kvs(const std::string& dir, const std::string& prefix,
                        const std::size_t instanceId, const std::size_t snapshotId)
{
    // Do nothing. Tail.
}

template <typename... TSnapshots>
inline void prepare_kvs(const std::string& dir, const std::string& prefix,
                        const std::size_t instanceId, const std::size_t snapshotId,
                        const std::string& snapshotData, TSnapshots&&... snapshots)
{
    const std::string path{dir + "/" + prefix + std::to_string(instanceId) + "_" + std::to_string(snapshotId)};
    const std::string jsonPath(path + ".json");
    const std::string hashPath(path + ".hash");

    store(jsonPath, hashPath, snapshotData);

    prepare_kvs(dir, prefix, instanceId, (snapshotId + 1ul), std::forward<TSnapshots>(snapshots)...);
}

template <typename... TSnapshots>
inline void prepare_environment(const std::string& dir, const std::string& prefix,
                                const std::size_t instanceId, const std::string& defaultData,
                                TSnapshots&&... snapshots)
{
    mkdir(dir.c_str(), 0777);

    const std::string path{dir + "/" + prefix + std::to_string(instanceId)};
    const std::string defaultJsonPath(path + "_default.json");
    const std::string defaultHashPath(path + "_default.hash");

    store(defaultJsonPath, defaultHashPath, defaultData);

    prepare_kvs(dir, prefix, instanceId, 0ul, std::forward<TSnapshots>(snapshots)...);
}

////////////////////////////////////////////////////////////////////////////////
/* Default data used in unittests*/
////////////////////////////////////////////////////////////////////////////////

const std::uint32_t instance = 123;
const InstanceId instance_id{instance};
const SnapshotId snapshot_id{0ul};

/* Notice: score::filesystem::Path could be constructed implicitly from std::string, but for
   readability, the explicit construction from those strings are used in the testcode */
const std::string data_dir = "./data_folder/";
const std::string default_prefix = data_dir + "kvs_" + std::to_string(instance) + "_default";
const std::string kvs_prefix = data_dir + "kvs_" + std::to_string(instance) + "_0";
const std::string filename_prefix = data_dir + "kvs_" + std::to_string(instance);

const std::string default_json = R"({
    "default": {
        "t": "i32",
        "v": 5
    }
})";
const std::string kvs_json = R"({
    "kvs": {
        "t": "i32",
        "v": 2
    }
})";

////////////////////////////////////////////////////////////////////////////////
/* Mock KvsValue for testing purposes (kvsvalue_to_any) */
////////////////////////////////////////////////////////////////////////////////

class BrokenKvsValue : public KvsValue
{
  public:
    BrokenKvsValue() : KvsValue(nullptr)
    {
        /* Intentionally break the type by assigning an invalid value */
        *(Type*)&this->type = static_cast<Type>(999);
    }
};

////////////////////////////////////////////////////////////////////////////////

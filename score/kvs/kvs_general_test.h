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
#ifndef SCORE_KVS_KVS_GENERAL_TEST_H
#define SCORE_KVS_KVS_GENERAL_TEST_H

// The test_kvs_general files provide configuration data and methods needed for KVS tests.

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

// Make private members public and drop final, to access member variables and derive from KvsValue in unit tests.
#define private public
#define final
#include "score/kvs/kvs_builder.h"
#undef private
#undef final
#include "score/filesystem/filesystem_mock.h"
#include "score/json/i_json_parser_mock.h"
#include "score/json/i_json_writer_mock.h"
#include "score/kvs/internal/kvs_helper.h"
using namespace score::mw::per::kvs;

uint32_t Adler32(const std::string& data);
void PrepareEnvironment();
void CleanupEnvironment();
void CleanupEnvironment(const std::string& dir);
void Store(const std::string& file_path, const std::string& hash_path, const std::string& data);

inline void PrepareKvs(const std::string& dir,
                       const std::string& prefix,
                       const std::size_t instance_id,
                       const std::size_t snapshot_id)
{
    // Do nothing, end of recursion.
}

template <typename... TSnapshots>
inline void PrepareKvs(const std::string& dir,
                       const std::string& prefix,
                       const std::size_t instance_id,
                       const std::size_t snapshot_id,
                       const std::string& snapshot_data,
                       TSnapshots&&... snapshots)
{
    const std::string path{dir + "/" + prefix + std::to_string(instance_id) + "_" + std::to_string(snapshot_id)};
    const std::string json_path{path + ".json"};
    const std::string hash_path{path + ".hash"};

    Store(json_path, hash_path, snapshot_data);

    PrepareKvs(dir, prefix, instance_id, (snapshot_id + 1ul), std::forward<TSnapshots>(snapshots)...);
}

template <typename... TSnapshots>
inline void PrepareEnvironment(const std::string& dir,
                               const std::string& prefix,
                               const std::size_t instance_id,
                               const std::string& default_data,
                               TSnapshots&&... snapshots)
{
    mkdir(dir.c_str(), 0777);

    const std::string path{dir + "/" + prefix + std::to_string(instance_id)};
    const std::string default_json_path{path + "_default.json"};
    const std::string default_hash_path{path + "_default.hash"};

    Store(default_json_path, default_hash_path, default_data);

    PrepareKvs(dir, prefix, instance_id, 0ul, std::forward<TSnapshots>(snapshots)...);
}

// Default data used in unit tests.

const std::uint32_t kInstance{123U};
const InstanceId kInstanceId{kInstance};
const SnapshotId kSnapshotId{0ul};

// score::filesystem::Path could be constructed implicitly from std::string.
// For readability, the test code constructs it explicitly from these strings.
const std::string kDataDir{"./data_folder/"};
const std::string kDefaultPrefix{kDataDir + "kvs_" + std::to_string(kInstance) + "_default"};
const std::string kKvsPrefix{kDataDir + "kvs_" + std::to_string(kInstance) + "_0"};
const std::string kFilenamePrefix{kDataDir + "kvs_" + std::to_string(kInstance)};

const std::string kDefaultJson{R"({
    "default": {
        "t": "i32",
        "v": 5
    }
})"};
const std::string kKvsJson{R"({
    "kvs": {
        "t": "i32",
        "v": 2
    }
})"};

#endif  // SCORE_KVS_KVS_GENERAL_TEST_H

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

#include "score/kvs/kvs_general_test.h"

/// @brief Independent Adler-32 implementation to check the KVS hash against.
uint32_t Adler32(const std::string& data)
{
    const uint32_t mod = 65521;
    uint32_t a = 1, b = 0;
    for (unsigned char c : data)
    {
        a = (a + c) % mod;
        b = (b + a) % mod;
    }
    return (b << 16) | a;
}

void Store(const std::string& file_path, const std::string& hash_path, const std::string& data)
{
    const uint32_t data_hash{Adler32(data)};

    std::ofstream file(file_path);
    if (file.is_open())
    {
        file << data;
        file.close();
    }

    std::ofstream hash_file(hash_path, std::ios::binary);
    if (hash_file.is_open())
    {
        hash_file.put((data_hash >> 24) & 0xFF);
        hash_file.put((data_hash >> 16) & 0xFF);
        hash_file.put((data_hash >> 8) & 0xFF);
        hash_file.put(data_hash & 0xFF);
        hash_file.close();
    }
}

/// @brief Creates the test environment with default data, which is needed in most test cases.
void PrepareEnvironment()
{
    // Prepare the test environment.
    mkdir(kDataDir.c_str(), 0777);

    const std::string default_json_path(kDefaultPrefix + ".json");
    const std::string default_hash_path(kDefaultPrefix + ".hash");
    const std::string kvs_json_path(kKvsPrefix + ".json");
    const std::string kvs_hash_path(kKvsPrefix + ".hash");

    Store(default_json_path, default_hash_path, kDefaultJson);
    Store(kvs_json_path, kvs_hash_path, kKvsJson);
}

void CleanupEnvironment()
{
    CleanupEnvironment(kDataDir);
}

void CleanupEnvironment(const std::string& dir)
{
    // Cleanup the test environment.
    if (std::filesystem::exists(dir))
    {
        for (auto& p : std::filesystem::recursive_directory_iterator(dir))
        {
            std::filesystem::permissions(p,
                                         std::filesystem::perms::owner_all | std::filesystem::perms::group_all |
                                             std::filesystem::perms::others_all,
                                         std::filesystem::perm_options::replace);
        }
        std::filesystem::remove_all(dir);
    }
}

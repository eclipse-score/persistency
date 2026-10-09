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
#include "kvs_general_test.hpp"

/* adler32 control instance */
uint32_t adler32(const std::string& data)
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

void store(const std::string& filePath, const std::string& hashPath, const std::string& data)
{
    const uint32_t dataHash{adler32(data)};

    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << data;
        file.close();
    }

    std::ofstream hashFile(hashPath, std::ios::binary);
    if (hashFile.is_open())
    {
        hashFile.put((dataHash >> 24) & 0xFF);
        hashFile.put((dataHash >> 16) & 0xFF);
        hashFile.put((dataHash >> 8) & 0xFF);
        hashFile.put(dataHash & 0xFF);
        hashFile.close();
    }
}

/* Create Test environment with default data, which is needed in most testcases */
void prepare_environment()
{
    /* Prepare the test environment */
    mkdir(data_dir.c_str(), 0777);

    const std::string default_json_path(default_prefix + ".json");
    const std::string default_hash_path(default_prefix + ".hash");
    const std::string kvs_json_path(kvs_prefix + ".json");
    const std::string kvs_hash_path(kvs_prefix + ".hash");

    store(default_json_path, default_hash_path, default_json);
    store(kvs_json_path, kvs_hash_path, kvs_json);
}

void cleanup_environment()
{
    cleanup_environment(data_dir);
}

void cleanup_environment(const std::string& dir)
{
    /* Cleanup the test environment */
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

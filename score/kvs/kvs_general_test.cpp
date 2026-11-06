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

#include "score/os/stat.h"

namespace
{
score::filesystem::Filesystem RealFilesystem()
{
    return score::filesystem::FilesystemFactory{}.CreateInstance();
}
}  // namespace

namespace filesystem_test
{
bool Exists(const std::string& path)
{
    const auto result{RealFilesystem().standard->Exists(score::filesystem::Path{path})};
    EXPECT_TRUE(result.has_value());
    return result.has_value() && result.value();
}

bool Remove(const std::string& path)
{
    const score::filesystem::Path filesystem_path{path};
    const auto exists_result{RealFilesystem().standard->Exists(filesystem_path)};
    EXPECT_TRUE(exists_result.has_value());
    if (!exists_result.has_value() || !exists_result.value())
    {
        return false;
    }

    const auto result{RealFilesystem().standard->Remove(filesystem_path)};
    EXPECT_TRUE(result.has_value());
    return result.has_value();
}

void RemoveAll(const std::string& path)
{
    const auto filesystem{RealFilesystem()};
    const score::filesystem::Path filesystem_path{path};
    const auto status{filesystem.standard->Status(filesystem_path)};
    EXPECT_TRUE(status.has_value());
    if (!status.has_value() || (status->Type() == score::filesystem::FileType::kNotFound))
    {
        return;
    }

    const auto result{filesystem.standard->RemoveAll(filesystem_path)};
    EXPECT_TRUE(result.has_value());
}

void CreateDirectory(const std::string& path)
{
    const auto result{RealFilesystem().standard->CreateDirectory(score::filesystem::Path{path})};
    EXPECT_TRUE(result.has_value());
}

void CreateDirectories(const std::string& path)
{
    const auto result{RealFilesystem().standard->CreateDirectories(score::filesystem::Path{path})};
    EXPECT_TRUE(result.has_value());
}

void SetPermissions(const std::string& path, const score::filesystem::Perms permissions)
{
    const auto result{RealFilesystem().standard->Permissions(
        score::filesystem::Path{path}, permissions, score::filesystem::PermOptions::kReplace)};
    EXPECT_TRUE(result.has_value());
}

std::size_t FileSize(const std::string& path)
{
    score::os::StatBuffer stat_buffer{};
    const auto result{score::os::Stat::instance().stat(score::filesystem::Path{path}.CStr(), stat_buffer)};
    EXPECT_TRUE(result.has_value());
    return result.has_value() ? static_cast<std::size_t>(stat_buffer.st_size) : 0U;
}
}  // namespace filesystem_test

/// @brief Independent Adler-32 implementation to check the KVS hash against.
uint32_t Adler32(const std::string& data)
{
    const uint32_t mod{65521U};
    uint32_t a{1U};
    uint32_t b{0U};
    for (unsigned char c : data)
    {
        a = (a + c) % mod;
        b = (b + a) % mod;
    }
    return (b << 16) | a;
}

/// @brief Creates the test environment with default data, which is needed in most test cases.
void PrepareEnvironment()
{
    // Prepare the test environment.
    mkdir(kDataDir.c_str(), 0777);

    const std::string default_json_path{kDefaultPrefix + ".json"};
    const std::string default_hash_path{kDefaultPrefix + ".hash"};
    const std::string kvs_json_path{kKvsPrefix + ".json"};
    const std::string kvs_hash_path{kKvsPrefix + ".hash"};

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
    const score::filesystem::Path path{dir};
    const auto filesystem{RealFilesystem()};
    const auto exists_result{filesystem.standard->Exists(path)};
    EXPECT_TRUE(exists_result.has_value());
    if (exists_result.has_value() && exists_result.value())
    {
        const auto all_permissions{score::filesystem::Perms::kReadWriteExecUser |
                                   score::filesystem::Perms::kReadWriteExecGroup |
                                   score::filesystem::Perms::kReadWriteExecOthers};
        for (const auto& entry : score::filesystem::RecursiveDirectoryIterator{path})
        {
            const auto permissions_result{filesystem.standard->Permissions(
                entry.GetPath(), all_permissions, score::filesystem::PermOptions::kReplace)};
            EXPECT_TRUE(permissions_result.has_value());
        }
        const auto remove_result{filesystem.standard->RemoveAll(path)};
        EXPECT_TRUE(remove_result.has_value());
    }
}

void Store(const std::string& file_path, const std::string& hash_path, const std::string& data)
{
    const uint32_t data_hash{Adler32(data)};

    std::ofstream file{file_path};
    if (file.is_open())
    {
        file << data;
        file.close();
    }

    std::ofstream hash_file{hash_path, std::ios::binary};
    if (hash_file.is_open())
    {
        hash_file.put((data_hash >> 24) & 0xFF);
        hash_file.put((data_hash >> 16) & 0xFF);
        hash_file.put((data_hash >> 8) & 0xFF);
        hash_file.put(data_hash & 0xFF);
        hash_file.close();
    }
}

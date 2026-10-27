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
/* Shared fixture for the configuration-by-file tests (kvs_config_parser_test.cpp and kvs_config_test.cpp).
 *
 * Every test runs in a freshly created, hermetic temporary directory. The environment variable
 * STORAGE_CONFIG_PATH is cleared before each test and restored to its previous state afterwards,
 * so tests never observe or leak the developer's environment.
 */
////////////////////////////////////////////////////////////////////////////////

#ifndef SCORE_LIB_KVS_KVS_CONFIG_GENERAL_TEST_HPP
#define SCORE_LIB_KVS_KVS_CONFIG_GENERAL_TEST_HPP

#include "score/kvs/error.hpp"
#include "score/kvs/internal/kvs_config_parser.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace score::mw::per::kvs::test
{

/* Instance id used by the configuration tests unless a test needs several instances. */
inline constexpr std::size_t kTestInstanceId = 7;

/* snapshot_max_count written by file entries in tests; differs from the built-in default 3. */
inline constexpr std::size_t kFileSnapshotMaxCount{1U};

/* Wraps a `key_value_storage_list` payload into a complete, valid-format configuration document. */
inline std::string Doc(const std::string& list)
{
    return R"({"format_version": 1, "key_value_storage_list": )" + list + "}";
}

/* Same as `Doc()` for a single entry of instance `kTestInstanceId`; `fields` are the remaining
 * members of the entry, e.g. `R"("storage_path": "./x")"`. */
inline std::string TestInstanceDoc(const std::string& fields = "")
{
    return Doc("[{\"instance_id\": " + std::to_string(kTestInstanceId) + (fields.empty() ? "" : ", " + fields) + "}]");
}

template <typename TResult>
void ExpectErrorCode(const TResult& result, ErrorCode expected)
{
    ASSERT_FALSE(result) << "expected error code " << static_cast<int>(expected) << " but call succeeded";
    EXPECT_EQ(static_cast<ErrorCode>(*result.error()), expected);
}

class KvsConfigFixture : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        const char* previous = std::getenv(kEnvVarName);
        had_previous_env_ = (previous != nullptr);
        if (had_previous_env_)
        {
            previous_env_ = previous;
        }
        ::unsetenv(kEnvVarName);

        const std::string tmpl = (std::filesystem::temp_directory_path() / "kvs_cfg_test_XXXXXX").string();
        std::vector<char> buffer(tmpl.begin(), tmpl.end());
        buffer.push_back('\0');
        ASSERT_NE(::mkdtemp(buffer.data()), nullptr);
        base_dir_ = buffer.data();
    }

    void TearDown() override
    {
        if (had_previous_env_)
        {
            ::setenv(kEnvVarName, previous_env_.c_str(), 1);
        }
        else
        {
            ::unsetenv(kEnvVarName);
        }
        std::error_code ec;
        std::filesystem::remove_all(base_dir_, ec);
    }

    /* Points STORAGE_CONFIG_PATH at `value` (may be a directory, a file, or empty). */
    void SetConfigEnv(const std::string& value)
    {
        ASSERT_EQ(::setenv(kEnvVarName, value.c_str(), 1), 0);
    }

    /* Creates (if needed) and returns the absolute path of a subdirectory of the temp base. */
    std::string SubDir(const std::string& name) const
    {
        const std::filesystem::path dir = std::filesystem::path(base_dir_) / name;
        std::filesystem::create_directories(dir);
        return dir.string();
    }

    /* Writes `content` to `<base>/<relative>` (creating parent dirs) and returns the path. */
    std::string WriteFile(const std::string& relative, const std::string& content) const
    {
        const std::filesystem::path path = std::filesystem::path(base_dir_) / relative;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << content;
        return path.string();
    }

    /* Writes a configuration document to `<base>/<relative>` and returns its path. */
    std::string WriteConfig(const std::string& content, const std::string& relative = "etc/cfg.json") const
    {
        return WriteFile(relative, content);
    }

    /* Sorted list of entry names directly inside `dir` (empty if the directory does not exist). */
    static std::vector<std::string> ListDir(const std::string& dir)
    {
        std::vector<std::string> names;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
        {
            names.push_back(entry.path().filename().string());
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    static std::string ReadFile(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }

    /* Maps file name -> content for every file directly inside `dir`. Used to prove that a
     * rejected operation left the storage byte-for-byte unchanged. */
    static std::vector<std::pair<std::string, std::string>> Snapshot(const std::string& dir)
    {
        std::vector<std::pair<std::string, std::string>> files;
        for (const auto& name : ListDir(dir))
        {
            files.emplace_back(name, ReadFile((std::filesystem::path(dir) / name).string()));
        }
        return files;
    }

    template <typename TResult>
    static ErrorCode CodeOf(const TResult& result)
    {
        return static_cast<ErrorCode>(*result.error());
    }

    /* The production constant is a string_view over a string literal, so data() is NUL-terminated. */
    static constexpr const char* kEnvVarName = kStorageConfigPathEnvVar.data();

    std::string base_dir_;

  private:
    bool had_previous_env_{false};
    std::string previous_env_;
};

} /* namespace score::mw::per::kvs::test */

#endif /* SCORE_LIB_KVS_KVS_CONFIG_GENERAL_TEST_HPP */

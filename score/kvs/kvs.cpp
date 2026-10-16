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
#include "score/kvs/kvs.hpp"
#include "internal/kvs_helper.hpp"
#include "score/kvs/error.hpp"
#include "score/os/errno.h"
#include "score/os/fcntl.h"
#include "score/os/stat.h"
#include "score/os/stdio.h"
#include "score/os/unistd.h"
#include <algorithm>
#include <cstdio>  // std::rename
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

// TODO Default Value Handling TBD
// TODO Add Score Logging
// TODO Replace std libs with baselibs
// TODO String Handling in set_value TBD: (e.g. set_value("key", "value") is not correct, since
// KvsValue() expects a string ptr and not a string)

using namespace std;

namespace score::mw::per::kvs
{

namespace
{
/* Size of a snapshot hash file in bytes (adler32 checksum) */
constexpr size_t HASH_FILE_SIZE = 4U;

/**
 * Validate that:
 * - Key length is in valid range - (0; KVS_MAX_KEY_LENGTH_BYTES>.
 * - Key consists solely of printable ASCII characters (0x20 - 0x7E).
 */
bool is_key_valid(const std::string_view key) noexcept
{
    // comp_req__kvs__key_length: disallow empty keys and keys too long.
    if (key.empty() || key.length() > KVS_MAX_KEY_LENGTH_BYTES)
    {
        return false;
    }

    // comp_req__kvs__key_naming: check for allowed characters.
    // comp_req__kvs__key_encoding: allowed characters enforce encoding.
    return std::all_of(key.cbegin(), key.cend(), [](char c) {
        return c >= 0x20 && c <= 0x7E;
    });
}

/* Permissions of KVS JSON and hash files: 0640 */
constexpr score::os::Stat::Mode KVS_FILE_MODE{score::os::Stat::Mode::kReadUser | score::os::Stat::Mode::kWriteUser |
                                              score::os::Stat::Mode::kReadGroup};

/* Path of the temporary file a KVS file is written to before it replaces the KVS file */
score::filesystem::Path temp_file_path(const score::filesystem::Path& path) noexcept
{
    return score::filesystem::Path{path.Native() + ".tmp"};
}

/* Remove a file, a missing file is not an error */
bool remove_if_exists(const score::filesystem::Path& path) noexcept
{
    const auto result = score::os::Unistd::instance().unlink(path.CStr());
    return result.has_value() || (result.error() == score::os::Error::Code::kNoSuchFileOrDirectory);
}
} /* namespace */

/*********************** KVS Implementation *********************/
Kvs::Kvs() noexcept
    : max_snapshots_(KVS_DEFAULT_MAX_SNAPSHOTS),
      filesystem_(std::make_unique<score::filesystem::Filesystem>(
          score::filesystem::FilesystemFactory{}.CreateInstance())) /* Create Filesystem instance, noexcept call */
      ,
      parser_(std::make_unique<score::json::JsonParser>()),
      writer_(std::make_unique<score::json::JsonWriter>()),
      logger_(std::make_unique<score::mw::log::Logger>("SKVS"))
{
}

Kvs::Kvs(Kvs&& other) noexcept
    : filename_prefix_(std::move(other.filename_prefix_)),
      max_snapshots_(other.max_snapshots_),
      filesystem_(std::move(other.filesystem_)),
      parser_(std::move(other.parser_)) /* Not absolutely necessary, because a new JSON writer/parser
                                         object would also be okay*/
      ,
      writer_(std::move(other.writer_)),
      logger_(std::move(other.logger_)),
      max_storage_bytes_(other.max_storage_bytes_)
{
    {
        std::lock_guard<std::mutex> lock(other.kvs_mutex_);
        kvs_ = std::move(other.kvs_);
    }

    default_values_ = std::move(other.default_values_);
}

Kvs& Kvs::operator=(Kvs&& other) noexcept
{
    if (this != &other)
    {
        filename_prefix_ = std::move(other.filename_prefix_);
        max_snapshots_ = other.max_snapshots_;

        {
            /* Lock both mutexes at once to avoid deadlock on concurrent cross-moves */
            std::scoped_lock lock(kvs_mutex_, other.kvs_mutex_);
            kvs_ = std::move(other.kvs_);
        }
        default_values_ = std::move(other.default_values_);

        filesystem_ = std::move(other.filesystem_);
        /* Transfer ownership of JSON parser and writer
            Not absolutely necessary, because a new JSON writer/parser object would also be okay*/
        parser_ = std::move(other.parser_);
        writer_ = std::move(other.writer_);
        logger_ = std::move(other.logger_);
        max_storage_bytes_ = other.max_storage_bytes_;
    }
    return *this;
}

/* Helper Function to parse JSON data for open_json*/
score::Result<Kvs::KeyValueMap> Kvs::parse_json_data(const std::string& data) noexcept
{
    auto any_res = parser_->FromBuffer(data);
    if (!any_res)
    {
        return score::MakeUnexpected(ErrorCode::JsonParserError);
    }

    score::json::Any root = std::move(any_res).value();
    auto obj = root.As<score::json::Object>();
    if (!obj.has_value())
    {
        return score::MakeUnexpected(ErrorCode::JsonParserError);
    }

    KeyValueMap result_value;
    for (const auto& element : obj.value().get())
    {
        auto sv = element.first.GetAsStringView();
        std::string key(sv.data(), sv.size());

        // Validate key.
        if (!is_key_valid(sv))
        {
            logger_->LogError() << "Provided key is not valid" << key;
            return score::MakeUnexpected(ErrorCode::InvalidKey);
        }

        auto conv = any_to_kvsvalue(element.second);
        if (!conv)
        {
            return score::MakeUnexpected(static_cast<ErrorCode>(*conv.error()));
        }
        result_value.emplace(std::move(key), std::move(conv.value()));
    }

    return result_value;
}

/* Open and read JSON File */
score::Result<Kvs::KeyValueMap> Kvs::open_json(const score::filesystem::Path& prefix,
                                               const OpenJsonNeedFile need_file) noexcept
{
    score::filesystem::Path json_file = prefix.Native() + ".json";
    score::filesystem::Path hash_file = prefix.Native() + ".hash";

    /* Read JSON file */
    ifstream in(json_file.CStr());
    if (!in)
    {
        if (need_file == OpenJsonNeedFile::Required)
        {
            logger_->LogError() << "error: file" << json_file << "could not be read";
            return score::MakeUnexpected(ErrorCode::KvsFileReadError);
        }
        else
        {
            logger_->LogInfo() << "file" << json_file << "not found, using empty data";
            return KeyValueMap{};
        }
    }

    ostringstream ss;
    ss << in.rdbuf();
    const std::string data = ss.str();

    /* Verify JSON Hash */
    ifstream hin(hash_file.CStr(), ios::binary);
    if (!hin)
    {
        logger_->LogError() << "error: hash file" << hash_file << "could not be read";
        return score::MakeUnexpected(ErrorCode::KvsHashFileReadError);
    }
    if (!check_hash(data, hin))
    {
        logger_->LogError() << "error: KVS data corrupted:" << json_file << hash_file;
        return score::MakeUnexpected(ErrorCode::ValidationFailed);
    }
    logger_->LogInfo() << "JSON data has valid hash";

    /* Parse JSON Data */
    auto parse_res = parse_json_data(data);
    if (!parse_res)
    {
        logger_->LogError() << "error: parsing JSON data failed";
        return score::MakeUnexpected(static_cast<ErrorCode>(*parse_res.error()));
    }

    return std::move(parse_res.value());
}

/* Open KVS Instance */
score::Result<Kvs> Kvs::open(const InstanceId& instance_id,
                             const SnapshotId& snapshot_id,
                             const OpenNeedDefaults& need_defaults,
                             const OpenNeedKvs& need_kvs,
                             const std::string&& dir,
                             const std::optional<size_t>& max_storage_bytes,
                             std::size_t snapshot_max_count) noexcept
{
    score::filesystem::Path base_path(dir);
    score::filesystem::Path filename_prefix = base_path / ("kvs_" + std::to_string(instance_id.id));
    const score::filesystem::Path filename_default = filename_prefix.Native() + "_default";
    const score::filesystem::Path filename_kvs = filename_prefix.Native() + "_" + std::to_string(snapshot_id.id);

    Kvs kvs;                                   /* Create KVS instance */
    kvs.max_storage_bytes_ = max_storage_bytes; /* Store maximum storage limit */
    score::Result<KeyValueMap> default_res{};

    if (need_defaults != OpenNeedDefaults::Ignored)
    {
        default_res = kvs.open_json(
            filename_default,
            need_defaults == OpenNeedDefaults::Required ? OpenJsonNeedFile::Required : OpenJsonNeedFile::Optional);
    }

    if (!default_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(
            *default_res.error())); /* Dereferences the Error class to its underlying code -> error.h*/
    }

    score::Result<KeyValueMap> kvs_res{};

    if (need_kvs != OpenNeedKvs::Ignored)
    {
        kvs_res = kvs.open_json(
            filename_kvs, need_kvs == OpenNeedKvs::Required ? OpenJsonNeedFile::Required : OpenJsonNeedFile::Optional);
    }

    if (!kvs_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*kvs_res.error()));
    }

    kvs.kvs_ = std::move(kvs_res.value());
    kvs.default_values_ = std::move(default_res.value());
    kvs.filename_prefix_ = filename_prefix;

    /* comp_req__kvs__snapshot_max_num_cfg: the maximum is limited to the range
       0..KVS_MAX_SNAPSHOTS_UPPER_BOUND. A larger request is clamped and reported,
       so that the caller can see the value it actually got. */
    kvs.max_snapshots_ = snapshot_max_count;
    if (snapshot_max_count > KVS_MAX_SNAPSHOTS_UPPER_BOUND)
    {
        kvs.logger_->LogWarn() << "snapshot_max_count" << snapshot_max_count << "exceeds upper bound"
                               << KVS_MAX_SNAPSHOTS_UPPER_BOUND << "- clamped";
        kvs.max_snapshots_ = KVS_MAX_SNAPSHOTS_UPPER_BOUND;
    }

    kvs.logger_->LogInfo() << "[instance=" << instance_id.id << "]"
                           << "[snapshot=" << snapshot_id.id << "]"
                           << "[maxSnapshotCount=" << kvs.max_snapshots_ << "] KVS opened";
    return kvs;
}

/* Reset KVS to initial state*/
score::Result<void> Kvs::reset() noexcept
{
    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    kvs_.clear();
    return {};
}

/* Retrieve all keys in the KVS*/
score::Result<std::vector<std::string>> Kvs::get_all_keys() noexcept
{
    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    std::vector<std::string> keys;
    keys.reserve(kvs_.size());
    for (const auto& [key, _] : kvs_)
    {
        keys.emplace_back(key);
    }
    return keys;
}

/* Check if a key exists*/
score::Result<bool> Kvs::key_exists(const std::string_view key) noexcept
{
    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    /* unordered_map find() needs string and doesnt work with string_view,
       workaround for c++20: heterogeneous lookup (applies to more functions) */
    return kvs_.find(std::string(key)) != kvs_.end();
}

/* Retrieve the value associated with a key*/
score::Result<KvsValue> Kvs::get_value(const std::string_view key) noexcept
{
    std::unique_lock<std::mutex> lock_kvs(kvs_mutex_, std::try_to_lock);
    if (!lock_kvs.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    auto search_kvs = kvs_.find(std::string(key));
    if (search_kvs == kvs_.end())
    {
        return score::MakeUnexpected(ErrorCode::KeyNotFound);
    }
    return search_kvs->second;
}

/*Retrieve the default value associated with a key*/
score::Result<KvsValue> Kvs::get_default_value(const std::string_view key) noexcept
{
    auto search = default_values_.find(std::string(key));
    if (search == default_values_.end())
    {
        return score::MakeUnexpected(ErrorCode::KeyNotFound);
    }
    return search->second;
}

/* Resets a Key to its default value (Deletes written key if default is available) */
score::Result<void> Kvs::reset_key(const std::string_view key) noexcept
{
    std::unique_lock<std::mutex> lock_kvs(kvs_mutex_, std::try_to_lock);
    if (!lock_kvs.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    if (default_values_.find(std::string(key)) == default_values_.end())
    {
        return score::MakeUnexpected(ErrorCode::KeyDefaultNotFound);
    }

    (void)kvs_.erase(std::string(key)); /* Return value ignored: a key that was never written needs no reset */
    return {};
}

/* Check if the value wasn't set yet and uses its default value */
score::Result<bool> Kvs::is_value_default(const std::string_view key) const noexcept
{
    std::string key_str{key};
    if (kvs_.find(key_str) != kvs_.end())
    {
        return false;
    }
    else if (default_values_.find(key_str) != default_values_.end())
    {
        return true;
    }
    else
    {
        return score::MakeUnexpected(ErrorCode::KeyNotFound);
    }
}

/* Set the value for a key*/
score::Result<void> Kvs::set_value(const std::string_view key, const KvsValue& value) noexcept
{
    // Validate key.
    if (!is_key_valid(key))
    {
        logger_->LogError() << "Provided key is not valid" << key;
        return score::MakeUnexpected(ErrorCode::InvalidKey);
    }

    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    kvs_.insert_or_assign(std::string(key), value);
    return {};
}

/* Remove a key-value pair*/
score::Result<void> Kvs::remove_key(const std::string_view key) noexcept
{
    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    if (kvs_.erase(std::string(key)) == 0U)
    {
        return score::MakeUnexpected(ErrorCode::KeyNotFound);
    }
    return {};
}

score::Result<void> Kvs::remove_all_keys() noexcept
{
    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    kvs_.clear();
    return {};
}

/* Drop all in-memory changes by reloading the persisted KVS file */
score::Result<void> Kvs::discard_pending_changes() noexcept
{
    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    /* Snapshot 0 is the current persisted state: written by flush(), read by open(). */
    const score::filesystem::Path kvs_path = filename_prefix_.Native() + "_0";

    /* Optional: a KVS opened without an existing file and never flushed discards to empty. */
    auto data_res = open_json(kvs_path, OpenJsonNeedFile::Optional);
    if (!data_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*data_res.error()));
    }

    kvs_ = std::move(data_res.value());
    return {};
}

/* Helper: size of one file in bytes. A missing file counts as 0 bytes. */
score::Result<size_t> Kvs::get_file_size(const score::filesystem::Path& file_path) noexcept
{
    std::error_code ec;
    const auto size = std::filesystem::file_size(file_path.CStr(), ec);

    if (ec)
    {
        /* Check if the error is "file not found" */
        if (ec == std::errc::no_such_file_or_directory)
        {
            /* File does not exist, its size is 0. This is not an error. */
            return 0;
        }
        else
        {
            logger_->LogError() << "Error: Could not get size of file" << file_path << ":" << ec.message();
            return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
        }
    }

    return size;
}

/* Helper: current storage size of all persisted files (defaults and historical snapshots) */
score::Result<size_t> Kvs::get_current_storage_size() noexcept
{
    size_t total_size = 0;
    const std::array<const char*, 2> file_extensions = {".json", ".hash"};

    /* Add the size of the default files */
    const std::string default_suffix = "_default";
    for (const char* extension : file_extensions)
    {
        const score::filesystem::Path file_path = filename_prefix_.Native() + default_suffix + extension;
        auto size_result = get_file_size(file_path);
        if (!size_result)
        {
            return size_result;
        }
        total_size += size_result.value();
    }

    /* Add the size of current and historical snapshots that will be kept after rotation (0 to N-1) */
    for (size_t snapshot_index = 0; snapshot_index < max_snapshots_; ++snapshot_index)
    {
        const std::string snapshot_suffix = "_" + to_string(snapshot_index);

        for (const char* extension : file_extensions)
        {
            const score::filesystem::Path file_path = filename_prefix_.Native() + snapshot_suffix + extension;
            auto size_result = get_file_size(file_path);
            if (!size_result)
            {
                return size_result;
            }
            total_size += size_result.value();
        }
    }
    return total_size;
}

/* Helper: write data to the temporary file of a KVS file and ensure it reaches physical storage.
   The live file is not touched, it is replaced by commit_temp_file() once the data is on storage. */
score::Result<void> Kvs::write_temp_file(const score::filesystem::Path& path, const void* data, std::size_t size) noexcept
{
    const score::os::Unistd& unistd = score::os::Unistd::instance();
    const score::filesystem::Path tmp_path = temp_file_path(path);

    /* Remove a temporary file left behind by an interrupted write */
    if (!remove_if_exists(tmp_path))
    {
        logger_->LogError() << "Failed to remove stale temporary file" << tmp_path;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    /* Create a new file: the mode is applied on creation and a file planted at the path is never opened */
    using Open = score::os::Fcntl::Open;
    const auto open_result = score::os::Fcntl::instance().open(
        tmp_path.CStr(), Open::kWriteOnly | Open::kCreate | Open::kExclusive | Open::kCloseOnExec, KVS_FILE_MODE);
    if (!open_result.has_value())
    {
        logger_->LogError() << "Failed to open file" << tmp_path;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }
    const std::int32_t fd = open_result.value();

    /* Close and remove the temporary file on failure */
    const auto discard = [&unistd, &tmp_path, fd]() {
        score::cpp::ignore = unistd.close(fd);
        score::cpp::ignore = unistd.unlink(tmp_path.CStr());
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    };

    /* Set the mode on the open file, the mode given on creation is restricted by umask */
    if (!score::os::Stat::instance().fchmod(fd, KVS_FILE_MODE).has_value())
    {
        logger_->LogError() << "Failed to set file permissions" << tmp_path;
        return discard();
    }

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    std::size_t written = 0U;
    while (written < size)
    {
        const auto write_result = unistd.write(fd, bytes + written, size - written);
        if (write_result.has_value() && (write_result.value() > 0))
        {
            written += static_cast<std::size_t>(write_result.value());
        }
        else if (write_result.has_value() ||
                 (write_result.error() != score::os::Error::Code::kOperationWasInterruptedBySignal))
        {
            logger_->LogError() << "Failed to write to file" << tmp_path;
            return discard();
        }
    }

    /* Request the OS to commit the data from its buffers to physical storage */
    if (!unistd.fdatasync(fd).has_value())
    {
        logger_->LogError() << "Failed to sync file" << tmp_path;
        return discard();
    }

    if (!unistd.close(fd).has_value())
    {
        logger_->LogError() << "Failed to close file" << tmp_path;
        score::cpp::ignore = unistd.unlink(tmp_path.CStr());
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    return {};
}

/* Helper: replace a KVS file with its temporary file. The rename is atomic, the file is either old or new. */
score::Result<void> Kvs::commit_temp_file(const score::filesystem::Path& path) noexcept
{
    const score::filesystem::Path tmp_path = temp_file_path(path);
    if (!score::os::Stdio::instance().rename(tmp_path.CStr(), path.CStr()).has_value())
    {
        logger_->LogError() << "Failed to replace file" << path;
        score::cpp::ignore = remove_if_exists(tmp_path);
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    return {};
}

/* Helper Function to write JSON data to a file for flush process (also adds Hash file)*/
score::Result<void> Kvs::write_json_data(const std::string& buf) noexcept
{
    score::filesystem::Path json_path{filename_prefix_.Native() + "_0.json"};
    score::filesystem::Path hash_path{filename_prefix_.Native() + "_0.hash"};
    score::filesystem::Path dir = json_path.ParentPath();
    if (dir.Empty())
    {
        logger_->LogError() << "Provided directory path is empty" << json_path;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    const auto create_path_res = filesystem_->standard->CreateDirectories(dir);
    if (!create_path_res.has_value())
    {
        logger_->LogError() << "Failed to create directory for KVS file" << json_path;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    /* Write JSON and hash files to temporary files, the live files stay untouched */
    auto json_res = write_temp_file(json_path, buf.data(), buf.size());
    if (!json_res.has_value())
    {
        return json_res;
    }

    std::array<uint8_t, 4> hash_bytes = get_hash_bytes(buf);
    auto hash_res = write_temp_file(hash_path, hash_bytes.data(), hash_bytes.size());
    if (!hash_res.has_value())
    {
        score::cpp::ignore = remove_if_exists(temp_file_path(json_path));
        return hash_res;
    }

    /* Replace live files with temporary files */
    auto json_commit_res = commit_temp_file(json_path);
    if (!json_commit_res.has_value())
    {
        score::cpp::ignore = remove_if_exists(temp_file_path(hash_path));
        return json_commit_res;
    }

    auto hash_commit_res = commit_temp_file(hash_path);
    if (!hash_commit_res.has_value())
    {
        return hash_commit_res;
    }

    /* Request the OS to commit the renames to physical storage */
    if (!filesystem_->utils->SyncDirectory(dir).has_value())
    {
        logger_->LogError() << "Failed to sync directory" << dir;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    return {};
}

/* Serialize the KVS and check the projected total against the storage limit */
score::Result<std::string> Kvs::serialize_and_check() noexcept
{
    score::json::Object root_obj;

    /* 1. Serialize the current KVS map */
    {
        std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
        if (!lock.owns_lock())
        {
            return score::MakeUnexpected(ErrorCode::MutexLockFailed);
        }
        for (const auto& [key, value] : kvs_)
        {
            auto conv = kvsvalue_to_any(value);
            if (!conv)
            {
                return score::MakeUnexpected<std::string>(conv.error());
            }
            root_obj.emplace(key, std::move(conv.value()));
        }
    }

    auto buf_res = writer_->ToBuffer(root_obj);
    if (!buf_res)
    {
        return score::MakeUnexpected(ErrorCode::JsonGeneratorError);
    }
    const std::string& buf = buf_res.value();

    /* Without a configured limit there is nothing to check, so the persisted files are not
       measured at all. flush() then behaves exactly as it does without this feature: no
       extra file system calls, and no new way to fail. */
    if (max_storage_bytes_.has_value())
    {
        /* 2. Size of all other persisted files */
        auto current_size_res = get_current_storage_size();
        if (!current_size_res)
        {
            return score::MakeUnexpected<std::string>(current_size_res.error());
        }

        /* 3. Projected total after this flush */
        const size_t total_size = current_size_res.value() + buf.size() + HASH_FILE_SIZE;

        /* 4. Check against the limit */
        if (total_size > max_storage_bytes_.value())
        {
            logger_->LogError() << "error: KVS storage limit would be exceeded. total_size:" << total_size
                               << "max_storage_bytes:" << max_storage_bytes_.value();
            return score::MakeUnexpected(ErrorCode::OutOfStorageSpace);
        }
    }

    return buf;
}

/* Flush the key-value store*/
score::Result<void> Kvs::flush() noexcept
{
    auto result = serialize_and_check();
    if (!result)
    {
        return score::MakeUnexpected<void>(result.error());
    }

    auto rotate_result = snapshot_rotate();
    if (!rotate_result)
    {
        return rotate_result;
    }

    return write_json_data(result.value());
}

/* Performs a 'dry run' to check the potential storage size */
score::Result<size_t> Kvs::calculate_potential_size() noexcept
{
    auto result = serialize_and_check();
    if (!result)
    {
        return score::MakeUnexpected<size_t>(result.error());
    }

    /* Re-calculate the size to return it, as serialize_and_check only returns the buffer */
    const std::string& buf = result.value();
    auto current_size_res = get_current_storage_size();
    if (!current_size_res)
    {
        return score::MakeUnexpected<size_t>(current_size_res.error());
    }

    return current_size_res.value() + buf.size() + HASH_FILE_SIZE;
}

/* Retrieve the snapshot count*/
score::Result<size_t> Kvs::snapshot_count() const noexcept
{
    size_t count = 0;
    for (size_t idx = 0; idx < max_snapshots_; ++idx)
    {
        const score::filesystem::Path fname = filename_prefix_.Native() + "_" + to_string(idx) + ".json";
        const auto fname_exists_res = filesystem_->standard->Exists(fname);
        if (!fname_exists_res)
        {
            return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
        }
        if (false == fname_exists_res.value())
        {
            break;
        }
        count = idx + 1;
    }

    return count;
}

/* Retrieve the max snapshot count*/
size_t Kvs::snapshot_max_count() const noexcept
{
    return max_snapshots_;
}

/* Rotate Snapshots */
score::Result<void> Kvs::snapshot_rotate() noexcept
{
    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    for (size_t idx = max_snapshots_; idx > 0; --idx)
    {
        score::filesystem::Path hash_old = filename_prefix_.Native() + "_" + to_string(idx - 1) + ".hash";
        score::filesystem::Path hash_new = filename_prefix_.Native() + "_" + to_string(idx) + ".hash";
        score::filesystem::Path snap_old = filename_prefix_.Native() + "_" + to_string(idx - 1) + ".json";
        score::filesystem::Path snap_new = filename_prefix_.Native() + "_" + to_string(idx) + ".json";

        logger_->LogInfo() << "rotating:" << snap_old << "->" << snap_new;
        /* Rename hash */
        int32_t hash_rename = std::rename(hash_old.CStr(), hash_new.CStr());
        if ((0 != hash_rename) && (errno != ENOENT))
        {
            logger_->LogError() << "error: could not rename hash file" << snap_old << "- Rename Errorcode" << errno;
            return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
        }

        /* Rename snapshot */
        int32_t snap_rename = std::rename(snap_old.CStr(), snap_new.CStr());
        if ((0 != snap_rename) && (errno != ENOENT))
        {
            logger_->LogError() << "error: could not rename snapshot file" << snap_old << "- Rename Errorcode" << errno;
            return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
        }
    }

    return {};
}

/* Restore the key-value store from a snapshot*/
score::Result<void> Kvs::snapshot_restore(const SnapshotId& snapshot_id) noexcept
{
    std::unique_lock<std::mutex> lock(kvs_mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    auto snapshot_count_res = snapshot_count();
    if (!snapshot_count_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*snapshot_count_res.error()));
    }

    /* Fail if the snapshot ID is the current KVS or does not exist */
    if ((0 == snapshot_id.id) || (snapshot_count_res.value() < snapshot_id.id))
    {
        return score::MakeUnexpected(ErrorCode::InvalidSnapshotId);
    }

    score::filesystem::Path restore_path = filename_prefix_.Native() + "_" + to_string(snapshot_id.id);
    auto data_res = open_json(restore_path, OpenJsonNeedFile::Required);
    if (!data_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*data_res.error()));
    }

    kvs_ = std::move(data_res.value());
    return {};
}

/* Get the filename for a snapshot*/
score::Result<score::filesystem::Path> Kvs::get_kvs_filename(const SnapshotId& snapshot_id) const noexcept
{
    score::filesystem::Path filename = filename_prefix_.Native() + "_" + std::to_string(snapshot_id.id) + ".json";

    const auto fname_exists_res = filesystem_->standard->Exists(filename);
    if (!fname_exists_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*fname_exists_res.error()));
    }
    if (false == fname_exists_res.value())
    {
        return score::MakeUnexpected(ErrorCode::FileNotFound);
    }
    return filename;
}

/* Get the hash filename for a snapshot*/
score::Result<score::filesystem::Path> Kvs::get_hash_filename(const SnapshotId& snapshot_id) const noexcept
{
    score::filesystem::Path filename = filename_prefix_.Native() + "_" + std::to_string(snapshot_id.id) + ".hash";

    const auto fname_exists_res = filesystem_->standard->Exists(filename);
    if (!fname_exists_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*fname_exists_res.error()));
    }
    if (false == fname_exists_res.value())
    {
        return score::MakeUnexpected(ErrorCode::FileNotFound);
    }
    return filename;
}

/* Get the combined on-disk size of the current KVS data and hash files */
score::Result<size_t> Kvs::get_storage_file_size() const noexcept
{
    const std::array<score::filesystem::Path, 2> paths{
        score::filesystem::Path{filename_prefix_.Native() + "_0.json"},
        score::filesystem::Path{filename_prefix_.Native() + "_0.hash"}};

    size_t total_size = 0;
    for (const auto& path : paths)
    {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path.CStr(), ec);
        if (!ec)
        {
            total_size += static_cast<size_t>(size);
        }
        else if (ec != std::errc::no_such_file_or_directory)
        {
            logger_->LogError() << "error: could not determine size of " << path << ": " << ec.message();
            return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
        }
    }

    return total_size;
}

} /* namespace score::mw::per::kvs */

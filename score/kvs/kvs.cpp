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
#include "internal/kvs_instance_state_registry.hpp"
#include "score/kvs/error.hpp"
#include <unistd.h>  // fileno(), fdatasync()
#include <algorithm>
#include <cstdio>  // std::fopen, std::fwrite, std::fflush, std::fclose
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

} /* namespace */

/*********************** KVS Implementation *********************/
Kvs::Kvs() noexcept
    : filesystem_(std::make_unique<score::filesystem::Filesystem>(score::filesystem::FilesystemFactory{}.CreateInstance())),
      parser_(std::make_unique<score::json::JsonParser>()),
      writer_(std::make_unique<score::json::JsonWriter>()),
      logger_(std::make_unique<score::mw::log::Logger>("SKVS"))
{
}

Kvs::Kvs(Kvs&& other) noexcept
    : state_(std::move(other.state_)),
      filesystem_(std::move(other.filesystem_)),
      parser_(std::move(other.parser_)),
      writer_(std::move(other.writer_)),
      logger_(std::move(other.logger_))
{
}

Kvs& Kvs::operator=(Kvs&& other) noexcept
{
    if (this != &other)
    {
        state_ = std::move(other.state_);
        filesystem_ = std::move(other.filesystem_);
        /* Transfer ownership of JSON parser and writer
            Not absolutely necessary, because a new JSON writer/parser object would also be okay*/
        parser_ = std::move(other.parser_);
        writer_ = std::move(other.writer_);
        logger_ = std::move(other.logger_);
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

    Kvs kvs;

    // Create an absolute path that that will be used as a hook for the given instance.
    const auto instance_path = kvs.filesystem_->standard->Absolute(filename_prefix);
    if (!instance_path)
    {
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    // Create a kvs state for the given instance.
    std::shared_ptr<KvsInstanceState> state{KvsStateRegistry::getInstance().getOrCreateState(instance_path.value().LexicallyNormal())};
    if (!state)
    {
        return score::MakeUnexpected(ErrorCode::InstanceCreationFailed);
    }

    std::unique_lock<std::mutex> lock(state->mutex_);

    // Already initialized.
    if (state->initialized_)
    {
        if (state->max_snapshots_ != snapshot_max_count ||
            state->max_storage_bytes_ != max_storage_bytes)
        {
            return score::MakeUnexpected(ErrorCode::InstanceParametersMismatch);
        }

        kvs.state_ = std::move(state);
        return std::move(kvs);
    }

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

    state->current_values_    = std::move(kvs_res.value());
    state->default_values_    = std::move(default_res.value());
    state->filename_prefix_   = std::move(filename_prefix);
    state->max_snapshots_     = KVS_DEFAULT_MAX_SNAPSHOTS;
    state->max_storage_bytes_ = max_storage_bytes;

    /* comp_req__kvs__snapshot_max_num_cfg: the maximum is limited to the range
       0..KVS_MAX_SNAPSHOTS_UPPER_BOUND. A larger request is clamped and reported,
       so that the caller can see the value it actually got. */
    state->max_snapshots_ = snapshot_max_count;
    if (snapshot_max_count > KVS_MAX_SNAPSHOTS_UPPER_BOUND)
    {
        kvs.logger_->LogWarn() << "snapshot_max_count" << snapshot_max_count << "exceeds upper bound"
                               << KVS_MAX_SNAPSHOTS_UPPER_BOUND << "- clamped";
        state->max_snapshots_ = KVS_MAX_SNAPSHOTS_UPPER_BOUND;
    }

    kvs.logger_->LogInfo() << "[instance=" << instance_id.id << "]"
                           << "[snapshot=" << snapshot_id.id << "]"
                           << "[maxSnapshotCount=" << state->max_snapshots_ << "] KVS opened";

    state->initialized_ = true;

    kvs.state_ = std::move(state);

    return kvs;
}

/* Reset KVS to initial state*/
score::Result<void> Kvs::reset() noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    state_->current_values_.clear();
    return {};
}

/* Retrieve all keys in the KVS*/
score::Result<std::vector<std::string>> Kvs::get_all_keys() noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    std::vector<std::string> keys;
    keys.reserve(state_->current_values_.size());
    for (const auto& [key, _] : state_->current_values_)
    {
        keys.emplace_back(key);
    }
    return keys;
}

/* Check if a key exists*/
score::Result<bool> Kvs::key_exists(const std::string_view key) noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    /* unordered_map find() needs string and doesnt work with string_view,
       workaround for c++20: heterogeneous lookup (applies to more functions) */
    return state_->current_values_.find(std::string(key)) != state_->current_values_.end();
}

/* Retrieve the value associated with a key*/
score::Result<KvsValue> Kvs::get_value(const std::string_view key) noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    auto search_kvs = state_->current_values_.find(std::string(key));
    if (search_kvs == state_->current_values_.end())
    {
        return score::MakeUnexpected(ErrorCode::KeyNotFound);
    }
    return search_kvs->second;
}

/*Retrieve the default value associated with a key*/
score::Result<KvsValue> Kvs::get_default_value(const std::string_view key) noexcept
{
    auto search = state_->default_values_.find(std::string(key));
    if (search == state_->default_values_.end())
    {
        return score::MakeUnexpected(ErrorCode::KeyNotFound);
    }
    return search->second;
}

/* Resets a Key to its default value (Deletes written key if default is available) */
score::Result<void> Kvs::reset_key(const std::string_view key) noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    if (state_->default_values_.find(std::string(key)) == state_->default_values_.end())
    {
        return score::MakeUnexpected(ErrorCode::KeyDefaultNotFound);
    }

    (void)state_->current_values_.erase(std::string(key)); /* Return value ignored: a key that was never written needs no reset */
    return {};
}

/* Check if the value wasn't set yet and uses its default value */
score::Result<bool> Kvs::is_value_default(const std::string_view key) const noexcept
{
    std::string key_str{key};
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (state_->current_values_.find(key_str) != state_->current_values_.end())
    {
        return false;
    }
    else if (state_->default_values_.find(key_str) != state_->default_values_.end())
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

    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    state_->current_values_.insert_or_assign(std::string(key), value);
    return {};
}

/* Remove a key-value pair*/
score::Result<void> Kvs::remove_key(const std::string_view key) noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    if (state_->current_values_.erase(std::string(key)) == 0U)
    {
        return score::MakeUnexpected(ErrorCode::KeyNotFound);
    }
    return {};
}

score::Result<void> Kvs::remove_all_keys() noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    state_->current_values_.clear();
    return {};
}

/* Drop all in-memory changes by reloading the persisted KVS file */
score::Result<void> Kvs::discard_pending_changes() noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    /* Snapshot 0 is the current persisted state: written by flush(), read by open(). */
    const score::filesystem::Path kvs_path = state_->filename_prefix_.Native() + "_0";

    /* Optional: a KVS opened without an existing file and never flushed discards to empty. */
    auto data_res = open_json(kvs_path, OpenJsonNeedFile::Optional);
    if (!data_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*data_res.error()));
    }

    state_->current_values_ = std::move(data_res.value());
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
        const score::filesystem::Path file_path = state_->filename_prefix_.Native() + default_suffix + extension;
        auto size_result = get_file_size(file_path);
        if (!size_result)
        {
            return size_result;
        }
        total_size += size_result.value();
    }

    /* Add the size of current and historical snapshots that will be kept after rotation (0 to N-1) */
    for (size_t snapshot_index = 0; snapshot_index < state_->max_snapshots_; ++snapshot_index)
    {
        const std::string snapshot_suffix = "_" + to_string(snapshot_index);

        for (const char* extension : file_extensions)
        {
            const score::filesystem::Path file_path = state_->filename_prefix_.Native() + snapshot_suffix + extension;
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

/* Helper: write data to a file and ensure it reaches physical storage.*/
score::Result<void> Kvs::write_and_sync(const std::string& path, const void* data, std::size_t size) noexcept
{
    auto file_deleter = [](std::FILE* f) {
        if (f != nullptr)
        {
            (void)std::fclose(f);
        }
    };
    std::unique_ptr<std::FILE, decltype(file_deleter)> file{std::fopen(path.c_str(), "wb"), file_deleter};

    if (file == nullptr)
    {
        logger_->LogError() << "Failed to open file" << path;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    if (std::fwrite(data, sizeof(char), size, file.get()) != size)
    {
        logger_->LogError() << "Failed to write to file" << path;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    /* Flush the buffer to the OS. */
    if (std::fflush(file.get()) != 0)
    {
        logger_->LogError() << "Failed to flush file" << path;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    /* Request the OS to commit the data from its buffers to physical storage */
    if (::fdatasync(::fileno(file.get())) != 0)
    {
        logger_->LogError() << "Failed to sync file" << path;
        return score::MakeUnexpected(ErrorCode::PhysicalStorageFailure);
    }

    return {};
}

/* Helper Function to write JSON data to a file for flush process (also adds Hash file)*/
score::Result<void> Kvs::write_json_data(const std::string& buf) noexcept
{
    score::filesystem::Path json_path{state_->filename_prefix_.Native() + "_0.json"};
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

    /* Write JSON file */
    auto json_res = write_and_sync(json_path.Native(), buf.data(), buf.size());
    if (!json_res.has_value())
    {
        return json_res;
    }

    /* Write Hash File */
    std::array<uint8_t, 4> hash_bytes = get_hash_bytes(buf);
    score::filesystem::Path fn_hash = state_->filename_prefix_.Native() + "_0.hash";

    return write_and_sync(fn_hash.Native(), hash_bytes.data(), hash_bytes.size());
}

/* Serialize the KVS and check the projected total against the storage limit */
score::Result<std::string> Kvs::serialize_and_check() noexcept
{
    score::json::Object root_obj;

    /* 1. Serialize the current KVS map */
    {
        std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
        if (!lock.owns_lock())
        {
            return score::MakeUnexpected(ErrorCode::MutexLockFailed);
        }
        for (const auto& [key, value] : state_->current_values_)
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
    if (state_->max_storage_bytes_.has_value())
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
        if (total_size > state_->max_storage_bytes_.value())
        {
            logger_->LogError() << "error: KVS storage limit would be exceeded. total_size:" << total_size
                                << "max_storage_bytes:" << state_->max_storage_bytes_.value();
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
    for (size_t idx = 0; idx < state_->max_snapshots_; ++idx)
    {
        const score::filesystem::Path fname = state_->filename_prefix_.Native() + "_" + to_string(idx) + ".json";
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
    return state_->max_snapshots_;
}

/* Rotate Snapshots */
score::Result<void> Kvs::snapshot_rotate() noexcept
{
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
    if (!lock.owns_lock())
    {
        return score::MakeUnexpected(ErrorCode::MutexLockFailed);
    }

    for (size_t idx = state_->max_snapshots_; idx > 0; --idx)
    {
        score::filesystem::Path hash_old = state_->filename_prefix_.Native() + "_" + to_string(idx - 1) + ".hash";
        score::filesystem::Path hash_new = state_->filename_prefix_.Native() + "_" + to_string(idx) + ".hash";
        score::filesystem::Path snap_old = state_->filename_prefix_.Native() + "_" + to_string(idx - 1) + ".json";
        score::filesystem::Path snap_new = state_->filename_prefix_.Native() + "_" + to_string(idx) + ".json";

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
    std::unique_lock<std::mutex> lock(state_->mutex_, std::try_to_lock);
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

    score::filesystem::Path restore_path = state_->filename_prefix_.Native() + "_" + to_string(snapshot_id.id);
    auto data_res = open_json(restore_path, OpenJsonNeedFile::Required);
    if (!data_res)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*data_res.error()));
    }

    state_->current_values_ = std::move(data_res.value());
    return {};
}

/* Get the filename for a snapshot*/
score::Result<score::filesystem::Path> Kvs::get_kvs_filename(const SnapshotId& snapshot_id) const noexcept
{
    score::filesystem::Path filename = state_->filename_prefix_.Native() + "_" + std::to_string(snapshot_id.id) + ".json";

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
    score::filesystem::Path filename = state_->filename_prefix_.Native() + "_" + std::to_string(snapshot_id.id) + ".hash";

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
        score::filesystem::Path{state_->filename_prefix_.Native() + "_0.json"},
        score::filesystem::Path{state_->filename_prefix_.Native() + "_0.hash"}};

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

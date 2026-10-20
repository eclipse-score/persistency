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

#include "score/kvs/kvs.h"

#include "score/kvs/internal/kvs_constants.h"
#include "score/kvs/internal/kvs_helper.h"
#include "score/kvs/internal/logger.h"
#include "score/os/errno.h"
#include "score/os/fcntl.h"
#include "score/os/stat.h"
#include "score/os/stdio.h"
#include "score/os/unistd.h"
#include "score/utility.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace score::kvs
{

namespace
{
/// @brief Size of a snapshot hash file in bytes (Adler-32 checksum).
constexpr size_t kHashFileSize{kHashSize};

/// @brief Permissions of KVS JSON and hash files: 0640.
constexpr score::os::Stat::Mode kKvsFileMode{score::os::Stat::Mode::kReadUser | score::os::Stat::Mode::kWriteUser |
                                             score::os::Stat::Mode::kReadGroup};

/// @brief Checks that a character is printable ASCII (0x20 - 0x7E).
bool IsPrintableAscii(const char c) noexcept
{
    const auto code{std::char_traits<char>::to_int_type(c)};
    return (code >= 0x20) && (code <= 0x7E);
}

/// @brief Checks that a key is valid.
///
/// A valid key:
/// - is 1 to kMaxKeyLengthBytes bytes long,
/// - consists solely of printable ASCII characters (0x20 - 0x7E).
bool IsKeyValid(const std::string_view key) noexcept
{
    // comp_req__kvs__key_length: disallow empty keys and keys too long.
    if (key.empty() || (key.length() > kMaxKeyLengthBytes))
    {
        return false;
    }

    // comp_req__kvs__key_naming: check for allowed characters.
    // comp_req__kvs__key_encoding: allowed characters enforce encoding.
    return std::all_of(key.cbegin(), key.cend(), IsPrintableAscii);
}

/// @brief Returns the path of the temporary file a KVS file is written to before it replaces the KVS file.
score::filesystem::Path TempFilePath(const score::filesystem::Path& path) noexcept
{
    return score::filesystem::Path{path.Native() + std::string{kTempSuffix}};
}

/// @brief Removes a file.
///
/// A missing file is not an error.
bool RemoveIfExists(const score::filesystem::Path& path) noexcept
{
    const auto result{score::os::Unistd::instance().unlink(path.CStr())};
    if (!result.has_value() && (result.error() != score::os::Error::Code::kNoSuchFileOrDirectory))
    {
        GetLogger().LogError() << "Failed to remove file" << path;
        return false;
    }
    return true;
}

/// @brief Retrieves the size of one file in bytes.
///
/// A missing file counts as 0 bytes.
score::Result<size_t> GetFileSize(const score::filesystem::Path& file_path,
                                  const score::filesystem::IStandardFilesystem& filesystem) noexcept
{
    score::os::StatBuffer stat_buffer{};
    const auto stat_result{score::os::Stat::instance().stat(file_path.CStr(), stat_buffer)};
    if (!stat_result)
    {
        if (stat_result.error() == score::os::Error::Code::kNoSuchFileOrDirectory)
        {
            return 0UL;
        }

        GetLogger().LogError() << "Error: Could not get size of file" << file_path << ":"
                               << stat_result.error().ToString();
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }

    const auto is_regular_file{filesystem.IsRegularFile(file_path)};
    // codeql::misra_deviation_next_line(kvs-bool-ref-load)
    if (!is_regular_file || !*is_regular_file || (stat_buffer.st_size < 0))
    {
        GetLogger().LogError() << "Error: Could not get size of regular file" << file_path;
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }

    return static_cast<size_t>(stat_buffer.st_size);
}

/// @brief Writes data to the temporary file of a KVS file and ensures it reaches physical storage.
///
/// The live file is not touched, it is replaced by CommitTempFile() once the data is on storage.
score::Result<void> WriteTempFile(const score::filesystem::Path& path,
                                  const void* data,
                                  const std::size_t size) noexcept
{
    const score::os::Unistd& unistd{score::os::Unistd::instance()};
    const score::filesystem::Path tmp_path{TempFilePath(path)};

    // Remove a temporary file left behind by an interrupted write.
    if (!RemoveIfExists(tmp_path))
    {
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }

    // Create a new file: the mode is applied on creation and a file planted at the path is never opened.
    using Open = score::os::Fcntl::Open;
    const auto open_result{score::os::Fcntl::instance().open(
        tmp_path.CStr(), Open::kWriteOnly | Open::kCreate | Open::kExclusive | Open::kCloseOnExec, kKvsFileMode)};
    if (!open_result.has_value())
    {
        GetLogger().LogError() << "Failed to open file" << tmp_path;
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }
    const std::int32_t fd{*open_result};

    // Close and remove the temporary file on failure.
    const auto discard = [&unistd, &tmp_path, fd]() {
        score::cpp::ignore = unistd.close(fd);
        // codeql::misra_deviation_next_line(kvs-bool-ref-binding)
        score::cpp::ignore = RemoveIfExists(tmp_path);
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    };

    // Set the mode on the open file, the mode given on creation is restricted by umask.
    if (!score::os::Stat::instance().fchmod(fd, kKvsFileMode).has_value())
    {
        GetLogger().LogError() << "Failed to set file permissions" << tmp_path;
        return discard();
    }

    // codeql::misra_deviation_next_line(kvs-write-byte-pointer)
    const auto* bytes{static_cast<const std::uint8_t*>(data)};
    std::size_t written{0U};
    while (written < size)
    {
        const auto write_result{unistd.write(fd, bytes + written, size - written)};
        if (write_result.has_value() && (*write_result > 0))
        {
            written += static_cast<std::size_t>(*write_result);
        }
        else if (write_result.has_value() ||
                 (write_result.error() != score::os::Error::Code::kOperationWasInterruptedBySignal))
        {
            GetLogger().LogError() << "Failed to write to file" << tmp_path;
            return discard();
        }
        else
        {
            // The write was interrupted by a signal before any data was written, so it is retried.
        }
    }

    // Request the OS to commit the data from its buffers to physical storage.
    if (!unistd.fdatasync(fd).has_value())
    {
        GetLogger().LogError() << "Failed to sync file" << tmp_path;
        return discard();
    }

    if (!unistd.close(fd).has_value())
    {
        GetLogger().LogError() << "Failed to close file" << tmp_path;
        // codeql::misra_deviation_next_line(kvs-bool-ref-binding)
        score::cpp::ignore = RemoveIfExists(tmp_path);
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }

    return {};
}

/// @brief Replaces a KVS file with its temporary file.
///
/// The rename is atomic, the file is either old or new.
score::Result<void> CommitTempFile(const score::filesystem::Path& path) noexcept
{
    const score::filesystem::Path tmp_path{TempFilePath(path)};
    if (!score::os::Stdio::instance().rename(tmp_path.CStr(), path.CStr()).has_value())
    {
        GetLogger().LogError() << "Failed to replace file" << path;
        // codeql::misra_deviation_next_line(kvs-bool-ref-binding)
        score::cpp::ignore = RemoveIfExists(tmp_path);
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }

    return {};
}
}  // namespace

Kvs::Kvs(Kvs&& other) noexcept
    : current_values_{std::move(other.current_values_)},
      default_values_{std::move(other.default_values_)},
      params_{std::move(other.params_)},
      filename_prefix_{std::move(other.filename_prefix_)},
      filesystem_{std::move(other.filesystem_)},
      parser_{std::move(other.parser_)},
      writer_{std::move(other.writer_)}
{
}

Kvs& Kvs::operator=(Kvs&& other) & noexcept
{
    if (this != &other)
    {
        current_values_ = std::move(other.current_values_);
        default_values_ = std::move(other.default_values_);
        params_ = std::move(other.params_);
        filename_prefix_ = std::move(other.filename_prefix_);
        filesystem_ = std::move(other.filesystem_);
        parser_ = std::move(other.parser_);
        writer_ = std::move(other.writer_);
    }
    return *this;
}

score::Result<void> Kvs::Reset() noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    current_values_.clear();
    return {};
}

score::Result<std::vector<std::string>> Kvs::GetAllKeys() const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    std::vector<std::string> keys;
    keys.reserve(current_values_.size());
    for (const auto& entry : current_values_)
    {
        score::cpp::ignore = keys.emplace_back(entry.first);
    }
    return keys;
}

score::Result<bool> Kvs::KeyExists(const std::string_view key) const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    // unordered_map find() needs string and doesn't work with string_view.
    // C++20 heterogeneous lookup would avoid the copy (applies to more functions).
    // codeql::misra_deviation_next_line(kvs-bool-ref-binding)
    return current_values_.find(std::string(key)) != current_values_.end();
}

score::Result<KvsValue> Kvs::GetValue(const std::string_view key) const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    const auto search{current_values_.find(std::string(key))};
    if (search == current_values_.end())
    {
        GetLogger().LogError() << "Key not found" << key;
        return score::MakeUnexpected(ErrorCode::kKeyNotFound);
    }
    return search->second;
}

score::Result<KvsValue> Kvs::GetDefaultValue(const std::string_view key) const noexcept
{
    const auto search{default_values_.find(std::string(key))};
    if (search == default_values_.end())
    {
        GetLogger().LogError() << "Key not found" << key;
        return score::MakeUnexpected(ErrorCode::kKeyNotFound);
    }
    return search->second;
}

score::Result<void> Kvs::SetValueToDefault(const std::string_view key) noexcept
{
    // Validate key.
    if (!IsKeyValid(key))
    {
        GetLogger().LogError() << "Provided key is not valid" << key;
        return score::MakeUnexpected(ErrorCode::kInvalidKey);
    }

    // Get a default value.
    auto default_value_result{GetDefaultValue(key)};
    if (!default_value_result.has_value())
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*default_value_result.error()));
    }

    // Set value.
    std::lock_guard<std::mutex> lock{mutex_};

    score::cpp::ignore = current_values_.insert_or_assign(std::string(key), std::move(*default_value_result));
    return {};
}

score::Result<bool> Kvs::IsValueDefault(const std::string_view key) const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    const std::string key_str{key};
    if (current_values_.find(key_str) != current_values_.end())
    {
        // codeql::misra_deviation_next_line(kvs-bool-ref-binding)
        return false;
    }
    else if (default_values_.find(key_str) != default_values_.end())
    {
        // codeql::misra_deviation_next_line(kvs-bool-ref-binding)
        return true;
    }
    else
    {
        GetLogger().LogError() << "Key not found" << key;
        return score::MakeUnexpected(ErrorCode::kKeyNotFound);
    }
}

score::Result<void> Kvs::SetValue(const std::string_view key, const KvsValue& value) noexcept
{
    // Validate key.
    if (!IsKeyValid(key))
    {
        GetLogger().LogError() << "Provided key is not valid" << key;
        return score::MakeUnexpected(ErrorCode::kInvalidKey);
    }

    // Validate value.
    if (!HasUniqueKeys(value))
    {
        GetLogger().LogError() << "Provided value has duplicate object keys" << key;
        return score::MakeUnexpected(ErrorCode::kInvalidValueType);
    }

    std::lock_guard<std::mutex> lock{mutex_};

    score::cpp::ignore = current_values_.insert_or_assign(std::string(key), value);
    return {};
}

score::Result<void> Kvs::RemoveKey(const std::string_view key) noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    if (current_values_.erase(std::string(key)) == 0U)
    {
        GetLogger().LogError() << "Key not found" << key;
        return score::MakeUnexpected(ErrorCode::kKeyNotFound);
    }
    return {};
}

score::Result<void> Kvs::RemoveAllKeys() noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    current_values_.clear();
    return {};
}

score::Result<void> Kvs::DiscardPendingChanges() noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    // Snapshot 0 is the current persisted state: written by Flush().
    const score::filesystem::Path kvs_path{filename_prefix_.Native() + std::string{kSnapshotSeparator} + "0"};

    // Optional: a KVS built without an existing file and never flushed discards to empty.
    auto data_result{OpenJson(kvs_path, OpenJsonNeedFile::kOptional)};
    if (!data_result)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*data_result.error()));
    }

    current_values_ = std::move(*data_result);
    return {};
}

score::Result<void> Kvs::Flush() noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    auto result{SerializeAndCheck()};
    if (!result)
    {
        return score::MakeUnexpected<void>(result.error());
    }

    auto rotate_result{SnapshotRotate()};
    if (!rotate_result)
    {
        return rotate_result;
    }

    return WriteJsonData(*result);
}

score::Result<size_t> Kvs::SnapshotCount() const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};
    return SnapshotCountInternal();
}

score::Result<size_t> Kvs::SnapshotCountInternal() const noexcept
{
    size_t count{0U};
    for (size_t index{0U}; index < params_.snapshot_count_limit; ++index)
    {
        const score::filesystem::Path filename{filename_prefix_.Native() + std::string{kSnapshotSeparator} +
                                               std::to_string(index) + std::string{kJsonSuffix}};
        const auto filename_exists_result{filesystem_->standard->Exists(filename)};
        if (!filename_exists_result)
        {
            GetLogger().LogError() << "Failed to check if file exists" << filename;
            return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
        }
        // codeql::misra_deviation_next_line(kvs-bool-ref-load)
        if (!*filename_exists_result)
        {
            break;
        }
        count = index + 1;
    }

    return count;
}

size_t Kvs::SnapshotMaxCount() const noexcept
{
    return params_.snapshot_count_limit;
}

score::Result<void> Kvs::SnapshotRestore(const SnapshotId& snapshot_id) noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    const auto snapshot_count_result{SnapshotCountInternal()};
    if (!snapshot_count_result)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*snapshot_count_result.error()));
    }

    // Fail if the snapshot ID is the current KVS or does not exist.
    if ((snapshot_id.id == 0U) || (*snapshot_count_result < snapshot_id.id))
    {
        GetLogger().LogError() << "Invalid snapshot ID" << snapshot_id.id;
        return score::MakeUnexpected(ErrorCode::kInvalidSnapshotId);
    }

    const score::filesystem::Path restore_path{filename_prefix_.Native() + std::string{kSnapshotSeparator} +
                                               std::to_string(snapshot_id.id)};
    auto data_result{OpenJson(restore_path, OpenJsonNeedFile::kRequired)};
    if (!data_result)
    {
        return score::MakeUnexpected(static_cast<ErrorCode>(*data_result.error()));
    }

    current_values_ = std::move(*data_result);
    return {};
}

score::Result<score::filesystem::Path> Kvs::KvsFilePath(const SnapshotId& snapshot_id) const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    score::filesystem::Path filename{filename_prefix_.Native() + std::string{kSnapshotSeparator} +
                                     std::to_string(snapshot_id.id) + std::string{kJsonSuffix}};

    const auto filename_exists_result{filesystem_->standard->Exists(filename)};
    if (!filename_exists_result)
    {
        GetLogger().LogError() << "Failed to check if file exists" << filename;
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }
    // codeql::misra_deviation_next_line(kvs-bool-ref-load)
    if (!*filename_exists_result)
    {
        GetLogger().LogError() << "File not found" << filename;
        return score::MakeUnexpected(ErrorCode::kFileNotFound);
    }
    return filename;
}

score::Result<score::filesystem::Path> Kvs::HashFilePath(const SnapshotId& snapshot_id) const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    score::filesystem::Path filename{filename_prefix_.Native() + std::string{kSnapshotSeparator} +
                                     std::to_string(snapshot_id.id) + std::string{kHashSuffix}};

    const auto filename_exists_result{filesystem_->standard->Exists(filename)};
    if (!filename_exists_result)
    {
        GetLogger().LogError() << "Failed to check if file exists" << filename;
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }
    // codeql::misra_deviation_next_line(kvs-bool-ref-load)
    if (!*filename_exists_result)
    {
        GetLogger().LogError() << "File not found" << filename;
        return score::MakeUnexpected(ErrorCode::kFileNotFound);
    }
    return filename;
}

score::Result<size_t> Kvs::GetStorageFileSize() const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    const std::array<score::filesystem::Path, 2> paths{
        score::filesystem::Path{filename_prefix_.Native() + std::string{kSnapshotSeparator} + "0" +
                                std::string{kJsonSuffix}},
        score::filesystem::Path{filename_prefix_.Native() + std::string{kSnapshotSeparator} + "0" +
                                std::string{kHashSuffix}}};

    size_t total_size{0U};
    for (const auto& path : paths)
    {
        const auto size_result{GetFileSize(path, *filesystem_->standard)};
        if (!size_result)
        {
            return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
        }
        total_size += *size_result;
    }

    return total_size;
}

score::Result<size_t> Kvs::CalculatePotentialSize() const noexcept
{
    std::lock_guard<std::mutex> lock{mutex_};

    auto result{SerializeAndCheck()};
    if (!result)
    {
        return score::MakeUnexpected<size_t>(result.error());
    }

    // Re-calculate the size to return it, as SerializeAndCheck() only returns the buffer.
    const std::string& buffer{*result};
    const auto current_size_result{GetCurrentStorageSize()};
    if (!current_size_result)
    {
        return score::MakeUnexpected<size_t>(current_size_result.error());
    }

    return *current_size_result + buffer.size() + kHashFileSize;
}

Kvs::Kvs() noexcept
    : filesystem_{
          std::make_unique<score::filesystem::Filesystem>(score::filesystem::FilesystemFactory{}.CreateInstance())},
      parser_{std::make_unique<score::json::JsonParser>()},
      writer_{std::make_unique<score::json::JsonWriter>()}
{
}

score::Result<void> Kvs::SnapshotRotate() const noexcept
{
    for (size_t index{params_.snapshot_count_limit}; index > 0; --index)
    {
        const score::filesystem::Path hash_old{filename_prefix_.Native() + std::string{kSnapshotSeparator} +
                                               std::to_string(index - 1) + std::string{kHashSuffix}};
        const score::filesystem::Path hash_new{filename_prefix_.Native() + std::string{kSnapshotSeparator} +
                                               std::to_string(index) + std::string{kHashSuffix}};
        const score::filesystem::Path snapshot_old{filename_prefix_.Native() + std::string{kSnapshotSeparator} +
                                                   std::to_string(index - 1) + std::string{kJsonSuffix}};
        const score::filesystem::Path snapshot_new{filename_prefix_.Native() + std::string{kSnapshotSeparator} +
                                                   std::to_string(index) + std::string{kJsonSuffix}};

        GetLogger().LogInfo() << "rotating:" << snapshot_old << "->" << snapshot_new;
        // Rename hash.
        const auto hash_rename{score::os::Stdio::instance().rename(hash_old.CStr(), hash_new.CStr())};
        if ((!hash_rename.has_value()) && (hash_rename.error() != score::os::Error::Code::kNoSuchFileOrDirectory))
        {
            GetLogger().LogError() << "error: could not rename hash file" << snapshot_old << "-"
                                   << hash_rename.error().ToString();
            return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
        }

        // Rename snapshot.
        const auto snapshot_rename{score::os::Stdio::instance().rename(snapshot_old.CStr(), snapshot_new.CStr())};
        if ((!snapshot_rename.has_value()) &&
            (snapshot_rename.error() != score::os::Error::Code::kNoSuchFileOrDirectory))
        {
            GetLogger().LogError() << "error: could not rename snapshot file" << snapshot_old << "-"
                                   << snapshot_rename.error().ToString();
            return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
        }
    }

    return {};
}

score::Result<KvsMap> Kvs::ParseJsonData(const std::string& data) const noexcept
{
    auto any_result{parser_->FromBuffer(data)};
    if (!any_result)
    {
        GetLogger().LogError() << "Failed to parse JSON data";
        return score::MakeUnexpected(ErrorCode::kJsonParserError);
    }

    score::json::Any root{std::move(*any_result)};
    const auto converted{AnyToKvsValue(root)};
    if (!converted)
    {
        GetLogger().LogError() << "Failed to convert JSON data";
        return score::MakeUnexpected(static_cast<ErrorCode>(*converted.error()));
    }
    if (converted->GetType() != KvsValue::Type::kObject)
    {
        GetLogger().LogError() << "JSON root is not an object";
        return score::MakeUnexpected(ErrorCode::kJsonParserError);
    }

    const auto* object{std::get_if<KvsValue::Object>(&converted->GetValue())};
    if (object == nullptr)
    {
        return score::MakeUnexpected(ErrorCode::kJsonParserError);
    }
    KvsMap map;
    map.reserve(object->size());
    for (const auto& member : *object)
    {
        // Validate key.
        if (!IsKeyValid(member.first))
        {
            GetLogger().LogError() << "Provided key is not valid" << member.first;
            return score::MakeUnexpected(ErrorCode::kInvalidKey);
        }
        score::cpp::ignore = map.emplace(member.first, member.second);
    }

    return map;
}

score::Result<KvsMap> Kvs::OpenJson(const score::filesystem::Path& prefix,
                                    const OpenJsonNeedFile need_file) const noexcept
{
    const score::filesystem::Path json_file{prefix.Native() + std::string{kJsonSuffix}};
    const score::filesystem::Path hash_file{prefix.Native() + std::string{kHashSuffix}};

    // Read JSON file.
    std::ifstream in{json_file.CStr()};
    if (!in)
    {
        if (need_file == OpenJsonNeedFile::kRequired)
        {
            GetLogger().LogError() << "error: file" << json_file << "could not be read";
            return score::MakeUnexpected(ErrorCode::kKvsFileReadError);
        }
        else
        {
            GetLogger().LogInfo() << "file" << json_file << "not found, using empty data";
            return KvsMap{};
        }
    }

    std::ostringstream stream;
    stream << in.rdbuf();
    const std::string data{stream.str()};

    // Verify JSON hash.
    std::ifstream hash_stream{hash_file.CStr(), std::ios::binary};
    if (!hash_stream)
    {
        GetLogger().LogError() << "error: hash file" << hash_file << "could not be read";
        return score::MakeUnexpected(ErrorCode::kKvsHashFileReadError);
    }
    if (!CheckHash(data, hash_stream))
    {
        GetLogger().LogError() << "error: KVS data corrupted:" << json_file << hash_file;
        return score::MakeUnexpected(ErrorCode::kValidationFailed);
    }
    GetLogger().LogInfo() << "JSON data has valid hash";

    // Parse JSON data.
    auto parse_result{ParseJsonData(data)};
    if (!parse_result)
    {
        GetLogger().LogError() << "error: parsing JSON data failed";
        return score::MakeUnexpected(static_cast<ErrorCode>(*parse_result.error()));
    }

    return std::move(*parse_result);
}

score::Result<void> Kvs::WriteJsonData(const std::string& buffer) const noexcept
{
    const score::filesystem::Path json_path{filename_prefix_.Native() + std::string{kSnapshotSeparator} + "0" +
                                            std::string{kJsonSuffix}};
    const score::filesystem::Path hash_path{filename_prefix_.Native() + std::string{kSnapshotSeparator} + "0" +
                                            std::string{kHashSuffix}};
    const score::filesystem::Path dir{json_path.ParentPath()};
    if (dir.Empty())
    {
        GetLogger().LogError() << "Provided directory path is empty" << json_path;
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }

    const auto create_path_result{filesystem_->standard->CreateDirectories(dir)};
    if (!create_path_result.has_value())
    {
        GetLogger().LogError() << "Failed to create directory for KVS file" << json_path;
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }

    // Write JSON and hash files to temporary files, the live files stay untouched.
    auto json_result{WriteTempFile(json_path, buffer.data(), buffer.size())};
    if (!json_result.has_value())
    {
        return json_result;
    }

    const std::array<uint8_t, kHashSize> hash_bytes{GetHashBytes(buffer)};
    auto hash_result{WriteTempFile(hash_path, hash_bytes.data(), hash_bytes.size())};
    if (!hash_result.has_value())
    {
        // codeql::misra_deviation_next_line(kvs-bool-ref-binding)
        score::cpp::ignore = RemoveIfExists(TempFilePath(json_path));
        return hash_result;
    }

    // Replace live files with temporary files.
    auto json_commit_result{CommitTempFile(json_path)};
    if (!json_commit_result.has_value())
    {
        // codeql::misra_deviation_next_line(kvs-bool-ref-binding)
        score::cpp::ignore = RemoveIfExists(TempFilePath(hash_path));
        return json_commit_result;
    }

    auto hash_commit_result{CommitTempFile(hash_path)};
    if (!hash_commit_result.has_value())
    {
        GetLogger().LogError() << "KVS file and hash file are inconsistent" << json_path;
        return hash_commit_result;
    }

    // Request the OS to commit the renames to physical storage.
    if (!filesystem_->utils->SyncDirectory(dir).has_value())
    {
        GetLogger().LogError() << "Failed to sync directory" << dir;
        return score::MakeUnexpected(ErrorCode::kPhysicalStorageFailure);
    }

    return {};
}

score::Result<std::string> Kvs::SerializeAndCheck() const noexcept
{
    // 1. Serialize the current KVS map.
    auto converted{KvsValueToAny(current_values_)};
    if (!converted)
    {
        GetLogger().LogError() << "Failed to serialize KVS data";
        return score::MakeUnexpected<std::string>(converted.error());
    }
    const score::json::Any root_obj{std::move(*converted)};

    auto buffer_result{writer_->ToBuffer(root_obj)};
    if (!buffer_result)
    {
        GetLogger().LogError() << "Failed to generate JSON data";
        return score::MakeUnexpected(ErrorCode::kJsonGeneratorError);
    }
    const std::string& buffer{*buffer_result};

    // Without a configured limit there is nothing to check, so the persisted files are not measured at all.
    // Flush() then behaves exactly as it does without this feature: no extra file system calls, and no new way to fail.
    if (params_.storage_quota)
    {
        // 2. Size of all other persisted files.
        const auto current_size_result{GetCurrentStorageSize()};
        if (!current_size_result)
        {
            return score::MakeUnexpected<std::string>(current_size_result.error());
        }

        // 3. Projected total after this flush.
        const size_t total_size{*current_size_result + buffer.size() + kHashFileSize};

        // 4. Check against the limit.
        if (total_size > *(params_.storage_quota))
        {
            GetLogger().LogError() << "error: KVS storage limit would be exceeded. total_size:" << total_size
                                   << "max_storage_bytes:" << *(params_.storage_quota);
            return score::MakeUnexpected(ErrorCode::kOutOfStorageSpace);
        }
    }

    return buffer;
}

score::Result<size_t> Kvs::GetCurrentStorageSize() const noexcept
{
    size_t total_size{0U};
    const std::array<std::string_view, 2> file_extensions{kJsonSuffix, kHashSuffix};

    // Add the size of the default files.
    const std::string default_suffix{kDefaultSuffix};
    for (const std::string_view extension : file_extensions)
    {
        const score::filesystem::Path file_path{filename_prefix_.Native() + default_suffix + std::string{extension}};
        auto size_result{GetFileSize(file_path, *filesystem_->standard)};
        if (!size_result)
        {
            return size_result;
        }
        total_size += *size_result;
    }

    // Add the size of current and historical snapshots that will be kept after rotation (0 to N-1).
    for (size_t snapshot_index{0U}; snapshot_index < params_.snapshot_count_limit; ++snapshot_index)
    {
        const std::string snapshot_suffix{std::string{kSnapshotSeparator} + std::to_string(snapshot_index)};

        for (const std::string_view extension : file_extensions)
        {
            const score::filesystem::Path file_path{filename_prefix_.Native() + snapshot_suffix +
                                                    std::string{extension}};
            auto size_result{GetFileSize(file_path, *filesystem_->standard)};
            if (!size_result)
            {
                return size_result;
            }
            total_size += *size_result;
        }
    }
    return total_size;
}

bool operator==(const KvsOpenParameters& v1, const KvsOpenParameters& v2) noexcept
{
    return (v1.current_load_policy == v2.current_load_policy) && (v1.default_load_policy == v2.default_load_policy) &&
           (v1.snapshot_id == v2.snapshot_id) && (v1.instance_id == v2.instance_id) &&
           (v1.snapshot_count_limit == v2.snapshot_count_limit) && (v1.storage_quota == v2.storage_quota) &&
           (v1.working_dir == v2.working_dir);
}

bool operator!=(const KvsOpenParameters& v1, const KvsOpenParameters& v2) noexcept
{
    return !(v1 == v2);
}

}  // namespace score::kvs

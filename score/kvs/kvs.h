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
#ifndef SCORE_KVS_KVS_H
#define SCORE_KVS_KVS_H

#include "score/filesystem/filesystem.h"
#include "score/json/json_parser.h"
#include "score/json/json_writer.h"
#include "score/kvs/error.h"
#include "score/kvs/kvs_value.h"
#include "score/mw/log/logger.h"
#include "score/result/result.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace score::mw::per::kvs
{

/// @brief Maximum allowed key length in bytes.
/// @note comp_req__kvs__key_length
constexpr std::size_t kMaxKeyLengthBytes = 32U;

/// @deprecated Use `kMaxKeyLengthBytes` instead.
[[deprecated("Use kMaxKeyLengthBytes instead. The snake_case API is removed in a future release.")]]
constexpr std::size_t KVS_MAX_KEY_LENGTH_BYTES = kMaxKeyLengthBytes;

/// @brief Upper bound of the configurable maximum number of snapshots.
///
/// The configurable maximum number of snapshots is limited to the range 0..kMaxSnapshotsUpperBound.
/// A value of 0 disables snapshot operations.
/// @note comp_req__kvs__snapshot_max_num_cfg
constexpr std::size_t kMaxSnapshotsUpperBound = 3U;

/// @deprecated Use `kMaxSnapshotsUpperBound` instead.
[[deprecated("Use kMaxSnapshotsUpperBound instead. The snake_case API is removed in a future release.")]]
constexpr std::size_t KVS_MAX_SNAPSHOTS_UPPER_BOUND = kMaxSnapshotsUpperBound;

/// @brief Default maximum number of snapshots.
///
/// It can be overridden per instance at init-time via KvsBuilder::SnapshotMaxCount().
/// @note comp_req__kvs__snapshot_max_num_cfg
constexpr std::size_t kDefaultMaxSnapshots = 3U;

/// @deprecated Use `kDefaultMaxSnapshots` instead.
[[deprecated("Use kDefaultMaxSnapshots instead. The snake_case API is removed in a future release.")]]
constexpr std::size_t KVS_DEFAULT_MAX_SNAPSHOTS = kDefaultMaxSnapshots;

/// @brief Identifier of a KVS instance.
struct InstanceId
{
    /// Numeric value of the identifier.
    size_t id;

    /// @brief Constructs an instance identifier.
    ///
    /// Not explicit to allow implicit construction, e.g. `function(0)` instead of `function(InstanceId(0))`.
    /// @param id Numeric value of the identifier.
    InstanceId(size_t id) noexcept
    {
        this->id = id;
    }
};

/// @brief Identifier of a KVS snapshot.
struct SnapshotId
{
    /// Numeric value of the identifier.
    size_t id;

    /// @brief Constructs a snapshot identifier.
    ///
    /// Not explicit to allow implicit construction, e.g. `function(0)` instead of `function(SnapshotId(0))`.
    /// @param id Numeric value of the identifier.
    SnapshotId(size_t id) noexcept
    {
        this->id = id;
    }
};

/// @brief Whether default values must be available when opening a KVS.
enum class OpenNeedDefaults : std::uint8_t
{
    /// Defaults must be available.
    kRequired = 0,
    /// Use empty defaults if none are available.
    kOptional = 1,
    /// Defaults are not loaded.
    kIgnored = 2,

    // Deprecated enumerators.
    // Each one is an alias of its k-prefixed replacement and is removed in a future release.
    Required [[deprecated("Use kRequired instead. The snake_case API is removed in a future release.")]] = kRequired,
    Optional [[deprecated("Use kOptional instead. The snake_case API is removed in a future release.")]] = kOptional,
    Ignored [[deprecated("Use kIgnored instead. The snake_case API is removed in a future release.")]] = kIgnored,
};

/// @brief Whether KVS data must already exist when opening a KVS.
enum class OpenNeedKvs : std::uint8_t
{
    /// KVS data must already exist.
    kRequired = 0,
    /// Use an empty KVS if no KVS data is available.
    kOptional = 1,
    /// KVS data is not loaded.
    kIgnored = 2,

    // Deprecated enumerators.
    // Each one is an alias of its k-prefixed replacement and is removed in a future release.
    Required [[deprecated("Use kRequired instead. The snake_case API is removed in a future release.")]] = kRequired,
    Optional [[deprecated("Use kOptional instead. The snake_case API is removed in a future release.")]] = kOptional,
    Ignored [[deprecated("Use kIgnored instead. The snake_case API is removed in a future release.")]] = kIgnored,
};

/// @brief Whether a JSON file must exist when it is opened.
enum class OpenJsonNeedFile : std::uint8_t
{
    /// The file must already exist.
    kRequired = 0,
    /// If the file doesn't exist, start with empty data.
    kOptional = 1,

    // Deprecated enumerators.
    // Each one is an alias of its k-prefixed replacement and is removed in a future release.
    Required [[deprecated("Use kRequired instead. The snake_case API is removed in a future release.")]] = kRequired,
    Optional [[deprecated("Use kOptional instead. The snake_case API is removed in a future release.")]] = kOptional,
};

/// @brief A thread-safe key-value store (KVS).
///
/// The Kvs class provides an interface for managing a key-value store with features such as:
/// - Support for default values.
/// - Snapshot management for persistence and restoration.
///
/// @note A Kvs object is not copyable, but it can be moved.
class Kvs final
{
  public:
    /// @brief Map of keys to their values.
    using KeyValueMap = std::unordered_map<std::string, KvsValue>;

    Kvs(const Kvs&) = delete;
    Kvs& operator=(const Kvs&) = delete;

    /// @brief Move constructor.
    Kvs(Kvs&& other) noexcept;

    /// @brief Move assignment operator.
    Kvs& operator=(Kvs&& other) noexcept;

    /// @brief Opens the key-value store with the specified instance ID and flags.
    ///
    /// This function initializes and opens the key-value store (KVS) for a given instance ID.
    /// It lets the caller specify whether default values and an existing KVS are required or optional.
    ///
    /// @note Instead of using Kvs::Open() directly, it is recommended to use the KvsBuilder class.
    ///
    /// @param instance_id The instance ID of the KVS.
    /// This uniquely identifies the KVS instance.
    /// @param snapshot_id The snapshot ID of the KVS.
    /// This uniquely identifies the KVS snapshot.
    /// @param need_defaults Whether default values are required, optional or ignored.
    /// @param need_kvs Whether the KVS data is required, optional or ignored.
    /// @param dir The directory path where the KVS files are located.
    /// Use "" or "." for the current directory.
    /// @param max_storage_bytes Optional maximum total storage size in bytes.
    /// Pass std::nullopt to enforce no storage limit.
    /// @param snapshot_max_count The maximum number of snapshots this instance maintains.
    /// Values above kMaxSnapshotsUpperBound are clamped to it (a warning is logged).
    /// A value of 0 disables snapshot operations.
    /// The current KVS data is still persisted by Flush().
    /// @return The opened Kvs object on success, or an ErrorCode on failure.
    static score::Result<Kvs> Open(const InstanceId& instance_id,
                                   const SnapshotId& snapshot_id,
                                   const OpenNeedDefaults& need_defaults,
                                   const OpenNeedKvs& need_kvs,
                                   const std::string&& dir,
                                   const std::optional<size_t>& max_storage_bytes,
                                   std::size_t snapshot_max_count) noexcept;

    /// @brief Resets the key-value store to its initial state.
    /// @return Nothing on success, or an ErrorCode on failure.
    score::Result<void> Reset() noexcept;

    /// @brief Retrieves all keys stored in the key-value store.
    ///
    /// It only retrieves the written keys, no default keys are returned.
    ///
    /// @return All written keys on success, or an ErrorCode on failure.
    score::Result<std::vector<std::string>> GetAllKeys() noexcept;

    /// @brief Checks if a key exists in the key-value store.
    ///
    /// If the key was never written it will always return false, even if a default value for the key is available.
    ///
    /// @param key The key to check for existence.
    /// @return `true` if the key exists and `false` if it does not on success, or an ErrorCode on failure.
    score::Result<bool> KeyExists(const std::string_view key) noexcept;

    /// @brief Retrieves the value associated with the specified key from the key-value store.
    /// @param key The key for which the value is to be retrieved.
    /// @return The retrieved value on success, or an ErrorCode on failure.
    score::Result<KvsValue> GetValue(const std::string_view key) noexcept;

    /// @brief Retrieves the default value associated with the specified key.
    ///
    /// If the key has a default value, the corresponding value is returned.
    /// Otherwise, an error code is provided to indicate the failure reason.
    ///
    /// @param key The key for which the default value is to be retrieved.
    /// @return The default value on success, or an ErrorCode on failure.
    score::Result<KvsValue> GetDefaultValue(const std::string_view key) noexcept;

    /// @brief Resets a specified key to its default value.
    ///
    /// If no default value is available, it returns an ErrorCode and doesn't delete the key.
    /// If no key was ever written, but a default value is available, it returns successfully.
    /// If a key was written and it has a default value, it deletes the written key.
    ///
    /// @param key The key to be reset to its default value.
    /// @return Nothing on success, or an ErrorCode on failure.
    score::Result<void> ResetKey(const std::string_view key) noexcept;

    /// @brief Checks if the specified key wasn't set yet and uses its default value.
    /// @param key The key to check.
    /// @return `true` if the key uses its default value on success, or an ErrorCode on failure.
    score::Result<bool> IsValueDefault(const std::string_view key) const noexcept;

    /// @brief Stores a key-value pair in the key-value store.
    ///
    /// The key length is limited to kMaxKeyLengthBytes (32) bytes.
    /// @note comp_req__kvs__key_length
    ///
    /// @param key The key associated with the value to be stored.
    /// @param value The value to be stored.
    /// @return Nothing on success, ErrorCode::kInvalidKey if the key is not valid, or another ErrorCode on failure.
    score::Result<void> SetValue(const std::string_view key, const KvsValue& value) noexcept;

    /// @brief Removes a key-value pair from the store based on the specified key.
    /// @param key The key of the key-value pair to be removed.
    /// @return Nothing on success, or an ErrorCode on failure.
    score::Result<void> RemoveKey(const std::string_view key) noexcept;

    /// @brief Removes all key-value pairs from the store.
    /// @return Nothing on success, or an ErrorCode on failure.
    score::Result<void> RemoveAllKeys() noexcept;

    /// @brief Discards all pending changes to the key-value store.
    ///
    /// Reloads the key-value pairs from storage, dropping every change since the last successful `Flush()`.
    /// If `Flush()` was never called, it drops every change since `Open()`.
    /// Default values are not affected, since they are read-only for this instance.
    /// A store that was opened without an existing KVS file and never flushed discards to empty.
    ///
    /// @return Nothing on success, or an ErrorCode on failure.
    /// Re-reading persisted data can also fail with `kKvsHashFileReadError`, `kValidationFailed` or `kJsonParserError`.
    score::Result<void> DiscardPendingChanges() noexcept;

    /// @brief Flushes the key-value store, writing all pending changes to the underlying storage.
    /// @return Nothing on success, or an ErrorCode on failure.
    score::Result<void> Flush() noexcept;

    /// @brief Retrieves the number of snapshots currently stored in the key-value store.
    /// @return The number of snapshots on success, or an ErrorCode on failure.
    score::Result<size_t> SnapshotCount() const noexcept;

    /// @brief Retrieves the maximum number of snapshots that can be stored.
    ///
    /// The limit is configured per instance via KvsBuilder::SnapshotMaxCount().
    /// It defaults to kDefaultMaxSnapshots and never exceeds kMaxSnapshotsUpperBound.
    ///
    /// @return The maximum number of snapshots.
    size_t SnapshotMaxCount() const noexcept;

    /// @brief Restores the state of the key-value store from a specified snapshot.
    /// @param snapshot_id The identifier of the snapshot to restore from.
    /// @return Nothing on success, or an ErrorCode on failure.
    score::Result<void> SnapshotRestore(const SnapshotId& snapshot_id) noexcept;

    /// @brief Retrieves the filename associated with a given snapshot ID.
    /// @param snapshot_id The identifier of the snapshot for which the filename is to be retrieved.
    /// @return The path of the snapshot's KVS file on success, or an ErrorCode on failure.
    score::Result<score::filesystem::Path> GetKvsFilename(const SnapshotId& snapshot_id) const noexcept;

    /// @brief Retrieves the filename of the hash file associated with a given snapshot ID.
    ///
    /// The hash file stores integrity information for the snapshot.
    ///
    /// @param snapshot_id The identifier of the snapshot for which the hash filename is requested.
    /// @return The path of the snapshot's hash file on success, or an ErrorCode on failure.
    score::Result<score::filesystem::Path> GetHashFilename(const SnapshotId& snapshot_id) const noexcept;

    /// @brief Retrieves the size of the persisted key-value store on disk.
    ///
    /// Returns the combined size in bytes of the current KVS data file and its hash file (snapshot 0).
    /// Rotated snapshots and the defaults files are not included.
    /// The size is read from storage, so it reflects the last successful `Flush()` and not pending in-memory changes.
    /// A missing file counts as 0 bytes, so a store that was never flushed reports 0 instead of an error.
    ///
    /// @return The combined size of both files in bytes on success.
    /// `ErrorCode::kPhysicalStorageFailure` if a file exists but its size cannot be determined.
    score::Result<size_t> GetStorageFileSize() const noexcept;

    /// @brief Performs a 'dry run' to check if flushing the in-memory store would exceed the storage limit.
    ///
    /// This function serializes the key-value data to a temporary buffer and calculates the potential total size.
    /// It checks this size against the configured `max_storage_bytes` limit.
    ///
    /// @return The estimated total size that the KVS would occupy after a flush on success.
    /// `kOutOfStorageSpace` if the limit would be exceeded.
    /// Another ErrorCode for other failures (e.g. serialization).
    score::Result<size_t> CalculatePotentialSize() noexcept;

    // Deprecated snake_case API.
    // Each entry forwards to its PascalCase replacement and is removed in a future release.

    /// @deprecated Use `Open()` instead.
    [[deprecated("Use Open() instead. The snake_case API is removed in a future release.")]]
    static score::Result<Kvs> open(const InstanceId& instance_id,
                                   const SnapshotId& snapshot_id,
                                   const OpenNeedDefaults& need_defaults,
                                   const OpenNeedKvs& need_kvs,
                                   const std::string&& dir,
                                   const std::optional<size_t>& max_storage_bytes,
                                   std::size_t snapshot_max_count) noexcept
    {
        return Kvs::Open(
            instance_id, snapshot_id, need_defaults, need_kvs, std::move(dir), max_storage_bytes, snapshot_max_count);
    }

    /// @deprecated Use `Reset()` instead.
    [[deprecated("Use Reset() instead. The snake_case API is removed in a future release.")]]
    score::Result<void> reset() noexcept
    {
        return Reset();
    }

    /// @deprecated Use `GetAllKeys()` instead.
    [[deprecated("Use GetAllKeys() instead. The snake_case API is removed in a future release.")]]
    score::Result<std::vector<std::string>> get_all_keys() noexcept
    {
        return GetAllKeys();
    }

    /// @deprecated Use `KeyExists()` instead.
    [[deprecated("Use KeyExists() instead. The snake_case API is removed in a future release.")]]
    score::Result<bool> key_exists(const std::string_view key) noexcept
    {
        return KeyExists(key);
    }

    /// @deprecated Use `GetValue()` instead.
    [[deprecated("Use GetValue() instead. The snake_case API is removed in a future release.")]]
    score::Result<KvsValue> get_value(const std::string_view key) noexcept
    {
        return GetValue(key);
    }

    /// @deprecated Use `GetDefaultValue()` instead.
    [[deprecated("Use GetDefaultValue() instead. The snake_case API is removed in a future release.")]]
    score::Result<KvsValue> get_default_value(const std::string_view key) noexcept
    {
        return GetDefaultValue(key);
    }

    /// @deprecated Use `ResetKey()` instead.
    [[deprecated("Use ResetKey() instead. The snake_case API is removed in a future release.")]]
    score::Result<void> reset_key(const std::string_view key) noexcept
    {
        return ResetKey(key);
    }

    /// @deprecated Use `IsValueDefault()` instead.
    [[deprecated("Use IsValueDefault() instead. The snake_case API is removed in a future release.")]]
    score::Result<bool> is_value_default(const std::string_view key) const noexcept
    {
        return IsValueDefault(key);
    }

    /// @deprecated Use `SetValue()` instead.
    [[deprecated("Use SetValue() instead. The snake_case API is removed in a future release.")]]
    score::Result<void> set_value(const std::string_view key, const KvsValue& value) noexcept
    {
        return SetValue(key, value);
    }

    /// @deprecated Use `RemoveKey()` instead.
    [[deprecated("Use RemoveKey() instead. The snake_case API is removed in a future release.")]]
    score::Result<void> remove_key(const std::string_view key) noexcept
    {
        return RemoveKey(key);
    }

    /// @deprecated Use `RemoveAllKeys()` instead.
    [[deprecated("Use RemoveAllKeys() instead. The snake_case API is removed in a future release.")]]
    score::Result<void> remove_all_keys() noexcept
    {
        return RemoveAllKeys();
    }

    /// @deprecated Use `DiscardPendingChanges()` instead.
    [[deprecated("Use DiscardPendingChanges() instead. The snake_case API is removed in a future release.")]]
    score::Result<void> discard_pending_changes() noexcept
    {
        return DiscardPendingChanges();
    }

    /// @deprecated Use `Flush()` instead.
    [[deprecated("Use Flush() instead. The snake_case API is removed in a future release.")]]
    score::Result<void> flush() noexcept
    {
        return Flush();
    }

    /// @deprecated Use `SnapshotCount()` instead.
    [[deprecated("Use SnapshotCount() instead. The snake_case API is removed in a future release.")]]
    score::Result<size_t> snapshot_count() const noexcept
    {
        return SnapshotCount();
    }

    /// @deprecated Use `SnapshotMaxCount()` instead.
    [[deprecated("Use SnapshotMaxCount() instead. The snake_case API is removed in a future release.")]]
    size_t snapshot_max_count() const noexcept
    {
        return SnapshotMaxCount();
    }

    /// @deprecated Use `SnapshotRestore()` instead.
    [[deprecated("Use SnapshotRestore() instead. The snake_case API is removed in a future release.")]]
    score::Result<void> snapshot_restore(const SnapshotId& snapshot_id) noexcept
    {
        return SnapshotRestore(snapshot_id);
    }

    /// @deprecated Use `GetKvsFilename()` instead.
    [[deprecated("Use GetKvsFilename() instead. The snake_case API is removed in a future release.")]]
    score::Result<score::filesystem::Path> get_kvs_filename(const SnapshotId& snapshot_id) const noexcept
    {
        return GetKvsFilename(snapshot_id);
    }

    /// @deprecated Use `GetHashFilename()` instead.
    [[deprecated("Use GetHashFilename() instead. The snake_case API is removed in a future release.")]]
    score::Result<score::filesystem::Path> get_hash_filename(const SnapshotId& snapshot_id) const noexcept
    {
        return GetHashFilename(snapshot_id);
    }

    /// @deprecated Use `GetStorageFileSize()` instead.
    [[deprecated("Use GetStorageFileSize() instead. The snake_case API is removed in a future release.")]]
    score::Result<size_t> get_storage_file_size() const noexcept
    {
        return GetStorageFileSize();
    }

    /// @deprecated Use `CalculatePotentialSize()` instead.
    [[deprecated("Use CalculatePotentialSize() instead. The snake_case API is removed in a future release.")]]
    score::Result<size_t> calculate_potential_size() noexcept
    {
        return CalculatePotentialSize();
    }

  private:
    /// @brief Constructs an empty KVS.
    ///
    /// Private to prevent direct instantiation, use Open() instead.
    Kvs() noexcept;

    /// Mutex guarding `kvs_`.
    std::mutex kvs_mutex_;

    /// Written key-value pairs.
    KeyValueMap kvs_;

    /// Default values, fixed after Open().
    KeyValueMap default_values_;

    /// Path prefix of the KVS files, including the instance ID.
    score::filesystem::Path filename_prefix_;

    /// Maximum number of snapshots maintained by this instance.
    /// @note comp_req__kvs__snapshot_max_num_cfg
    std::size_t max_snapshots_;

    /// Filesystem handler for file operations.
    std::unique_ptr<score::filesystem::Filesystem> filesystem_;

    /// JSON parser for reading KVS data.
    std::unique_ptr<score::json::IJsonParser> parser_;

    /// JSON writer for writing KVS data.
    std::unique_ptr<score::json::IJsonWriter> writer_;

    /// Logger of this instance.
    std::unique_ptr<score::mw::log::Logger> logger_;

    /// Maximum storage limit in bytes.
    /// Unset means no limit is enforced.
    std::optional<size_t> max_storage_bytes_;

    /// @brief Rotates the snapshots, ensuring that the maximum count is maintained.
    score::Result<void> SnapshotRotate() noexcept;

    /// @brief Parses JSON data into a map of key-value pairs.
    score::Result<KeyValueMap> ParseJsonData(const std::string& data) noexcept;

    /// @brief Opens a JSON file, verifies its hash and returns its contents as a map of key-value pairs.
    score::Result<KeyValueMap> OpenJson(const score::filesystem::Path& prefix,
                                        const OpenJsonNeedFile need_file) noexcept;

    /// @brief Writes the provided data to the current JSON file and its hash file.
    score::Result<void> WriteJsonData(const std::string& buf) noexcept;

    /// @brief Writes data to the temporary file of a KVS file and ensures it reaches physical storage.
    ///
    /// The live file is not touched, it is replaced by CommitTempFile() once the data is on storage.
    score::Result<void> WriteTempFile(const score::filesystem::Path& path, const void* data, std::size_t size) noexcept;

    /// @brief Replaces a KVS file with its temporary file.
    ///
    /// The rename is atomic, the file is either old or new.
    score::Result<void> CommitTempFile(const score::filesystem::Path& path) noexcept;

    /// @brief Serializes the KVS and checks the projected total size against the storage limit.
    score::Result<std::string> SerializeAndCheck() noexcept;

    /// @brief Retrieves the size of one file in bytes.
    ///
    /// A missing file counts as 0 bytes.
    score::Result<size_t> GetFileSize(const score::filesystem::Path& file_path) noexcept;

    /// @brief Retrieves the current storage size of all persisted files (defaults and snapshots).
    score::Result<size_t> GetCurrentStorageSize() noexcept;
};

}  // namespace score::mw::per::kvs

#endif  // SCORE_KVS_KVS_H

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
#include "score/kvs/kvs_types.h"
#include "score/kvs/kvs_value.h"
#include "score/result/result.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace score::kvs
{
// TODO: Introduce a public KVS interface and make Kvs its hidden implementation.
/// @brief Persistent key-value store (KVS) instance.
///
/// A KVS instance holds key-value pairs and read-only default values in memory and persists its data to disk on
/// `Flush()`, keeping a configurable number of snapshots. Access default values with `GetDefaultValue()`.
/// Operations on a valid instance are thread-safe; moving an object must not occur concurrently with its use.
///
/// Shared state:
/// Matching builds for an instance ID return shared pointers to the same `Kvs` object.
/// A change made through any pointer, including a pending one, is immediately visible through all others.
/// In particular, `Reset()`, `RemoveAllKeys()`, `DiscardPendingChanges()`, `SnapshotRestore()` and
/// `Flush()` affect the shared instance. The registry retains the object until `score::kvs::Unregister()` or
/// process exit.
///
/// Opening:
/// Shared pointers are obtained with `KvsBuilder::Build()`. The first build of an instance ID loads its data from disk;
/// later builds reuse the same registered object without reading the files again. All builder options of a later build
/// must match those of the first one, otherwise the build fails with `kInstanceParametersMismatch`.
///
/// Ownership:
/// A `Kvs` object is movable but not copyable. Do not move from or move-assign to an object held by the registry.
/// A moved-from object may only be destroyed or assigned to.
// codeql::misra_deviation_next_line(kvs-special-members)
class Kvs final
{
  public:
    Kvs(const Kvs&) = delete;

    /// @brief Move constructor.
    /// @param other The object to move from.
    /// It must not be used afterwards, except to destroy it or assign a new value to it.
    Kvs(Kvs&& other) noexcept;

    Kvs& operator=(const Kvs&) = delete;

    /// @brief Move assignment operator.
    /// @param other The object to move from.
    /// It must not be used afterwards, except to destroy it or assign a new value to it.
    Kvs& operator=(Kvs&& other) & noexcept;

    /// @brief Resets the key-value store to its initial state.
    /// @return Nothing on success, or an ErrorCode on failure.
    score::Result<void> Reset() noexcept;

    /// @brief Retrieves all keys stored in the key-value store.
    ///
    /// It only retrieves the written keys, no default keys are returned.
    ///
    /// @return All written keys on success, or an ErrorCode on failure.
    score::Result<std::vector<std::string>> GetAllKeys() const noexcept;

    /// @brief Checks if a key exists in the key-value store.
    ///
    /// If the key was never written it will always return false, even if a default value for the key is available.
    ///
    /// @param key The key to check for existence.
    /// @return `true` if the key exists and `false` if it does not on success, or an ErrorCode on failure.
    score::Result<bool> KeyExists(const std::string_view key) const noexcept;

    /// @brief Retrieves the value associated with the specified key from the key-value store.
    /// @param key The key for which the value is to be retrieved.
    /// @return The retrieved value on success, or an ErrorCode on failure.
    score::Result<KvsValue> GetValue(const std::string_view key) const noexcept;

    /// @brief Retrieves the default value associated with the specified key.
    ///
    /// If the key has a default value, the corresponding value is returned.
    /// Otherwise, an error code is provided to indicate the failure reason.
    ///
    /// @param key The key for which the default value is to be retrieved.
    /// @return The default value on success, or an ErrorCode on failure.
    score::Result<KvsValue> GetDefaultValue(const std::string_view key) const noexcept;

    /// @brief Sets a specified key to its default value.
    ///
    /// If no default value is available, it returns an ErrorCode.
    ///
    /// @param key The key to be set to its default value.
    /// @return Nothing on success, or an ErrorCode on failure.
    score::Result<void> SetValueToDefault(const std::string_view key) noexcept;

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
    /// If `Flush()` was never called, it drops every change since the KVS was built.
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
    score::Result<score::filesystem::Path> KvsFilePath(const SnapshotId& snapshot_id) const noexcept;

    /// @brief Retrieves the filename of the hash file associated with a given snapshot ID.
    ///
    /// The hash file stores integrity information for the snapshot.
    ///
    /// @param snapshot_id The identifier of the snapshot for which the hash filename is requested.
    /// @return The path of the snapshot's hash file on success, or an ErrorCode on failure.
    score::Result<score::filesystem::Path> HashFilePath(const SnapshotId& snapshot_id) const noexcept;

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
    score::Result<size_t> CalculatePotentialSize() const noexcept;

    // Deprecated snake_case API.
    // Each entry forwards to its PascalCase replacement and is removed in a future release.

    /// @deprecated Use `Reset()` instead.
    [[deprecated("Use Reset() instead. This name is removed in a future release.")]]
    score::Result<void> reset() noexcept
    {
        return Reset();
    }

    /// @deprecated Use `GetAllKeys()` instead.
    [[deprecated("Use GetAllKeys() instead. This name is removed in a future release.")]]
    score::Result<std::vector<std::string>> get_all_keys() const noexcept
    {
        return GetAllKeys();
    }

    /// @deprecated Use `KeyExists()` instead.
    [[deprecated("Use KeyExists() instead. This name is removed in a future release.")]]
    score::Result<bool> key_exists(const std::string_view key) const noexcept
    {
        return KeyExists(key);
    }

    /// @deprecated Use `GetValue()` instead.
    [[deprecated("Use GetValue() instead. This name is removed in a future release.")]]
    score::Result<KvsValue> get_value(const std::string_view key) const noexcept
    {
        return GetValue(key);
    }

    /// @deprecated Use `GetDefaultValue()` instead.
    [[deprecated("Use GetDefaultValue() instead. This name is removed in a future release.")]]
    score::Result<KvsValue> get_default_value(const std::string_view key) const noexcept
    {
        return GetDefaultValue(key);
    }

    /// @deprecated Use `IsValueDefault()` instead.
    [[deprecated("Use IsValueDefault() instead. This name is removed in a future release.")]]
    score::Result<bool> is_value_default(const std::string_view key) const noexcept
    {
        return IsValueDefault(key);
    }

    /// @deprecated Use `SetValue()` instead.
    [[deprecated("Use SetValue() instead. This name is removed in a future release.")]]
    score::Result<void> set_value(const std::string_view key, const KvsValue& value) noexcept
    {
        return SetValue(key, value);
    }

    /// @deprecated Use `RemoveKey()` instead.
    [[deprecated("Use RemoveKey() instead. This name is removed in a future release.")]]
    score::Result<void> remove_key(const std::string_view key) noexcept
    {
        return RemoveKey(key);
    }

    /// @deprecated Use `RemoveAllKeys()` instead.
    [[deprecated("Use RemoveAllKeys() instead. This name is removed in a future release.")]]
    score::Result<void> remove_all_keys() noexcept
    {
        return RemoveAllKeys();
    }

    /// @deprecated Use `DiscardPendingChanges()` instead.
    [[deprecated("Use DiscardPendingChanges() instead. This name is removed in a future release.")]]
    score::Result<void> discard_pending_changes() noexcept
    {
        return DiscardPendingChanges();
    }

    /// @deprecated Use `Flush()` instead.
    [[deprecated("Use Flush() instead. This name is removed in a future release.")]]
    score::Result<void> flush() noexcept
    {
        return Flush();
    }

    /// @deprecated Use `SnapshotCount()` instead.
    [[deprecated("Use SnapshotCount() instead. This name is removed in a future release.")]]
    score::Result<size_t> snapshot_count() const noexcept
    {
        return SnapshotCount();
    }

    /// @deprecated Use `SnapshotMaxCount()` instead.
    [[deprecated("Use SnapshotMaxCount() instead. This name is removed in a future release.")]]
    size_t snapshot_max_count() const noexcept
    {
        return SnapshotMaxCount();
    }

    /// @deprecated Use `SnapshotRestore()` instead.
    [[deprecated("Use SnapshotRestore() instead. This name is removed in a future release.")]]
    score::Result<void> snapshot_restore(const SnapshotId& snapshot_id) noexcept
    {
        return SnapshotRestore(snapshot_id);
    }

    /// @deprecated Use `KvsFilePath()` instead.
    [[deprecated("Use KvsFilePath() instead. This name is removed in a future release.")]]
    score::Result<score::filesystem::Path> get_kvs_filename(const SnapshotId& snapshot_id) const noexcept
    {
        return KvsFilePath(snapshot_id);
    }

    /// @deprecated Use `HashFilePath()` instead.
    [[deprecated("Use HashFilePath() instead. This name is removed in a future release.")]]
    score::Result<score::filesystem::Path> get_hash_filename(const SnapshotId& snapshot_id) const noexcept
    {
        return HashFilePath(snapshot_id);
    }

    /// @deprecated Use `GetStorageFileSize()` instead.
    [[deprecated("Use GetStorageFileSize() instead. This name is removed in a future release.")]]
    score::Result<size_t> get_storage_file_size() const noexcept
    {
        return GetStorageFileSize();
    }

    /// @deprecated Use `CalculatePotentialSize()` instead.
    [[deprecated("Use CalculatePotentialSize() instead. This name is removed in a future release.")]]
    score::Result<size_t> calculate_potential_size() const noexcept
    {
        return CalculatePotentialSize();
    }

  private:
    friend class KvsBuilder;

    /// @brief Constructs an empty KVS.
    ///
    /// Private to prevent direct instantiation; use KvsBuilder::Build() instead.
    Kvs() noexcept;

    /// @brief Counts the existing snapshots. Caller must hold `mutex_`.
    score::Result<size_t> SnapshotCountInternal() const noexcept;

    /// @brief Rotates the snapshots, ensuring that the maximum count is maintained.
    score::Result<void> SnapshotRotate() const noexcept;

    /// @brief Parses JSON data into a map of key-value pairs.
    score::Result<KvsMap> ParseJsonData(const std::string& data) const noexcept;

    /// @brief Opens a JSON file, verifies its hash and returns its contents as a map of key-value pairs.
    score::Result<KvsMap> OpenJson(const score::filesystem::Path& prefix,
                                   const OpenJsonNeedFile need_file) const noexcept;

    /// @brief Writes the provided data to the current JSON file and its hash file.
    score::Result<void> WriteJsonData(const std::string& buffer) const noexcept;

    /// @brief Serializes the KVS and checks the projected total size against the storage limit.
    score::Result<std::string> SerializeAndCheck() const noexcept;

    /// @brief Retrieves the current storage size of all persisted files (defaults and snapshots).
    score::Result<size_t> GetCurrentStorageSize() const noexcept;

    /// Protects the instance state.
    mutable std::mutex mutex_;
    /// Values in the current snapshot.
    KvsMap current_values_;
    /// Default values loaded for this store.
    KvsMap default_values_;
    /// Parameters that have been used to open this instance.
    KvsOpenParameters params_;
    /// Prefix used to construct this store's file paths.
    score::filesystem::Path filename_prefix_;

    /// Filesystem handler for file operations.
    std::unique_ptr<score::filesystem::Filesystem> filesystem_;

    /// JSON parser for reading KVS data.
    std::unique_ptr<score::json::IJsonParser> parser_;

    /// JSON writer for writing KVS data.
    std::unique_ptr<score::json::IJsonWriter> writer_;
};

}  // namespace score::kvs

#endif  // SCORE_KVS_KVS_H

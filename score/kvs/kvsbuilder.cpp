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

#include "score/kvs/kvsbuilder.hpp"
#include "score/kvs/internal/kvs_config_parser_factory.hpp"
#include "score/kvs/internal/kvs_config_types.hpp"

namespace score::mw::per::kvs
{

static_assert(kSnapshotMaxCountLimit == KVS_MAX_SNAPSHOTS_UPPER_BOUND,
              "snapshot_max_count limit out of sync with KVS_MAX_SNAPSHOTS_UPPER_BOUND");

KvsBuilder::KvsBuilder(const InstanceId& instance_id) noexcept
    : instance_id_(instance_id),
      snapshot_id_(SnapshotId{0}),
      need_defaults_(OpenNeedDefaults::Optional),
      need_kvs_(OpenNeedKvs::Optional),
      directory_("./data_folder/"),
      maximum_storage_bytes_(std::nullopt),
      max_snapshots_(KVS_DEFAULT_MAX_SNAPSHOTS),
      read_only_setting_(false)
{
}

KvsBuilder& KvsBuilder::snapshot(const SnapshotId& snapshot_id) noexcept
{
    this->snapshot_id_ = snapshot_id;
    return *this;
}

KvsBuilder& KvsBuilder::need_defaults_flag(const OpenNeedDefaults& flag) noexcept
{
    need_defaults_ = flag;
    return *this;
}

KvsBuilder& KvsBuilder::need_kvs_flag(const OpenNeedKvs& flag) noexcept
{
    need_kvs_ = flag;
    return *this;
}

KvsBuilder& KvsBuilder::dir(const std::string& dir_path) noexcept
{
    this->directory_ = dir_path;
    return *this;
}

KvsBuilder& KvsBuilder::max_storage_bytes(const std::optional<size_t>& max_storage_bytes) noexcept
{
    this->maximum_storage_bytes_ = max_storage_bytes;
    return *this;
}

KvsBuilder& KvsBuilder::snapshot_max_count(const std::size_t max_count) noexcept
{
    this->max_snapshots_ = max_count;
    return *this;
}

KvsBuilder& KvsBuilder::read_only_flag(bool flag) noexcept
{
    read_only_setting_ = flag;
    return *this;
}

/// @note comp_req__kvs__constraints_priority
/*
 * input: none (reads the builder's own members and, through `IKvsConfigParser::load_entry()`,
 *        the configuration-by-file document applicable to `instance_id`, if any).
 * behavior: Loads the configuration-by-file entry matching `instance_id`. Each field present in that
 *           entry overwrites the corresponding builder member in place (file entry > builder setter
 *           > built-in default); no new parameters are created. Fields absent from the entry keep
 *           their builder value. Because of this, a later `build()` on the same builder sees the
 *           file values. An empty directory resolves to the current directory ("./") for the
 *           `Kvs::open()` call only. The directory is passed as a copy, so the builder's
 *           `directory` member is not moved from. A configuration-by-file `snapshot_max_count` is
 *           already validated to 0..KVS_MAX_SNAPSHOTS_UPPER_BOUND by `KvsConfigParser` and never
 *           reaches `Kvs::open()` out of range; only a builder-set (non-file) value relies on
 *           `Kvs::open()` clamping it.
 * output: the opened `Kvs` on success; the error propagated from `IKvsConfigParser::load_entry()`
 *         (`ErrorCode::FileNotFound`, `ErrorCode::KvsFileReadError`, `ErrorCode::JsonParserError`,
 *         or `ErrorCode::InvalidConfiguration`), or any error from `Kvs::open()`.
 */
score::Result<Kvs> KvsBuilder::build() noexcept
{
    auto parser = create_kvs_config_parser();
    auto entry_res = parser->load_entry(instance_id_.id);
    if (!entry_res)
    {
        return score::MakeUnexpected<Kvs>(entry_res.error());
    }

    if (entry_res.value().has_value())
    {
        const KvsConfigEntry& entry = entry_res.value().value();
        if (entry.storage_path.has_value())
        {
            directory_ = entry.storage_path.value();
        }
        if (entry.need_defaults.has_value())
        {
            need_defaults_ = (entry.need_defaults.value() == KvsConfigNeedMode::Required) ? OpenNeedDefaults::Required
                                                                                          : OpenNeedDefaults::Optional;
        }
        if (entry.need_kvs.has_value())
        {
            need_kvs_ =
                (entry.need_kvs.value() == KvsConfigNeedMode::Required) ? OpenNeedKvs::Required : OpenNeedKvs::Optional;
        }
        if (entry.snapshot_max_count.has_value())
        {
            max_snapshots_ = entry.snapshot_max_count.value();
        }
        if (entry.read_only.has_value())
        {
            read_only_setting_ = entry.read_only.value();
        }
        if (entry.max_storage_size_bytes.has_value())
        {
            maximum_storage_bytes_ = entry.max_storage_size_bytes;
        }
    }

    return Kvs::open(instance_id_,
                     snapshot_id_,
                     need_defaults_,
                     need_kvs_,
                     std::string(directory_.empty() ? "./" : directory_),
                     maximum_storage_bytes_,
                     max_snapshots_,
                     read_only_setting_);
}

}  // namespace score::mw::per::kvs

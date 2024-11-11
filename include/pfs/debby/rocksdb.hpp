////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "keyvalue_database.hpp"
#include <pfs/filesystem.hpp>

DEBBY__NAMESPACE_BEGIN

namespace rocksdb {

    struct options_type 
    {
        bool optimize {true};  // IncreaseParallelism and OptimizeLevelStyleCompaction
        bool small_db {false}; // like under 1GB
        int keep_log_file_num {10};
    };

    /**
    * Open database specified by @a path and create it if
    * missing when @a create_if_missing set to @c true.
    *
    * @param path Path to the database.
    *
    * @throw debby::error().
    */
    DEBBY__EXPORT
    keyvalue_database<backend_enum::rocksdb>
    make_kv (pfs::filesystem::path const & path, options_type opts, bool create_if_missing, error * perr = nullptr);

    DEBBY__EXPORT
    keyvalue_database<backend_enum::rocksdb>
    make_kv (pfs::filesystem::path const & path, bool create_if_missing, error * perr = nullptr);

    DEBBY__EXPORT bool wipe (pfs::filesystem::path const & path, error * perr = nullptr);
} // namespace rocksdb

template<>
template <typename ...Args>
keyvalue_database<backend_enum::rocksdb>
keyvalue_database<backend_enum::rocksdb>::make (Args &&... args)
{
    return keyvalue_database<backend_enum::rocksdb>{rocksdb::make_kv(std::forward<Args>(args)...)};
}

/**
* Deletes files associated with database
*/
template<>
template <typename ...Args>
bool keyvalue_database<backend_enum::rocksdb>::wipe (Args &&... args)
{
    return rocksdb::wipe(std::forward<Args>(args)...);
}

DEBBY__NAMESPACE_END

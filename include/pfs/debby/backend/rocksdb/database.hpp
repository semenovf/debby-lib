////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2023.02.08 Applied new API.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/debby/exports.hpp"
#include "pfs/filesystem.hpp"
#include <vector>

namespace rocksdb {
class DB;
class ColumnFamilyHandle;
struct Options;
} // namespace rocksdb

namespace debby {
namespace backend {
namespace rocksdb {

struct database
{
    using key_type     = std::string;
    using native_type  = ::rocksdb::DB *;
    using options_type = ::rocksdb::Options;

    template <typename T>
    union fixed_packer
    {
        T value;
        char bytes[sizeof(T)];
    };

    struct rep_type
    {
        native_type dbh;
        pfs::filesystem::path path;
    };

    /**
     * Open database specified by @a path.
     *
     * @param path Path to the database.
     * @param opts RocksDB specific options or @c nullptr for default options.
     *
     * @throw debby::error().
     */
    static DEBBY__EXPORT rep_type make_kv (pfs::filesystem::path const & path
        , options_type * popts, error * perr = nullptr);

    /**
     * Open database specified by @a path and create it if missing when
     * @a create_if_missing set to @c true.
     *
     * @param path Path to the database.
     * @param create_if_missing Create database if missing.
     * @param optimize_for_small_db Set to @c true if DB is very small (like
     *        under 1GB) and don't want to spend lots of memory for memtables.
     */
    static DEBBY__EXPORT rep_type make_kv (pfs::filesystem::path const & path
        , bool create_if_missing, bool optimize_for_small_db = true, error * perr = nullptr);

    static DEBBY__EXPORT bool wipe (pfs::filesystem::path const & path, error * perr = nullptr);
};

}}} // namespace debby::backend::rocksdb

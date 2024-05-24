////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2023.02.07 Applied new API.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "statement.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/statement.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/optional.hpp>
#include <cstdint>
#include <unordered_map>

struct sqlite3;
struct sqlite3_stmt;

namespace debby {
namespace backend {
namespace sqlite3 {

struct database
{
    using native_type    = struct sqlite3 *;
    using statement_type = debby::statement<debby::backend::sqlite3::statement>;
    using cache_type     = std::unordered_map<std::string, struct sqlite3_stmt *>;

    // Key-Value database traits
    using key_type = std::string;

    struct rep_type
    {
        native_type dbh;
        cache_type  cache; // Prepared statements cache
    };

    // https://www.sqlite.org/pragma.html#pragma_journal_mode
    enum journal_mode_enum { JM_DELETE, JM_TRUNCATE, JM_PERSIST, JM_MEMORY, JM_WAL, JM_OFF };

    // https://www.sqlite.org/pragma.html#pragma_synchronous
    enum synchronous_enum { SYN_OFF, SYN_NORMAL, SYN_FULL, SYN_EXTRA };

    // https://www.sqlite.org/pragma.html#pragma_temp_store
    enum temp_store_enum { TS_DEFAULT, TS_FILE, TS_MEMORY };

    enum presets_enum
    {
          DEFAULT_PRESET
        , CONCURRENCY_PRESET
    };

    struct make_options
    {
        pfs::optional<journal_mode_enum> pragma_journal_mode;
        pfs::optional<synchronous_enum> pragma_synchronous;
        pfs::optional<temp_store_enum> pragma_temp_store;
        pfs::optional<std::size_t> pragma_mmap_size;
    };

    /**
     * Open database specified by @a path and create it if
     * missing when @a create_if_missing set to @c true.
     *
     * @details If the filename is an empty string, then a private, temporary
     *          on-disk database will be created. This private database will be
     *          automatically deleted as soon as the database connection is
     *          closed.
     *          If the filename is ":memory:", then a private, temporary
     *          in-memory database is created for the connection. This in-memory
     *          database will vanish when the database connection is closed.
     *          Future versions of SQLite might make use of additional special
     *          filenames that begin with the ":" character. It is recommended
     *          that when a database filename actually does begin with a ":"
     *          character you should prefix the filename with a pathname such
     *          as "./" to avoid ambiguity.
     *          For more details see @c sqlite3 documentation (sqlite3_open_v2).
     *
     * @param path Path to the database.
     * @param create_if_missing If @c true create database if it missing.
     */
    static DEBBY__EXPORT rep_type make_r (pfs::filesystem::path const & path
        , bool create_if_missing = true, error * perr = nullptr);

    static DEBBY__EXPORT rep_type make_r (pfs::filesystem::path const & path
        , bool create_if_missing, make_options && opts, error * perr = nullptr);

    static DEBBY__EXPORT rep_type make_r (pfs::filesystem::path const & path
        , bool create_if_missing, presets_enum preset, error * perr = nullptr);

    static DEBBY__EXPORT rep_type make_kv (pfs::filesystem::path const & path
        , bool create_if_missing = true, error * perr = nullptr);

    static DEBBY__EXPORT bool wipe (pfs::filesystem::path const & path, error * perr = nullptr);
};

}}} // namespace debby::backend::sqlite3

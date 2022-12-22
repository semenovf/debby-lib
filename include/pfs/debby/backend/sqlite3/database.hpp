////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "statement.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/statement.hpp"
#include "pfs/filesystem.hpp"
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
    using key_type     = std::string;

    enum {
          INT_COLUMN_FAMILY_INDEX = 0
        , FP_COLUMN_FAMILY_INDEX
        , STR_COLUMN_FAMILY_INDEX
        , BLOB_COLUMN_FAMILY_INDEX
    };

    template <typename T>
    union fixed_packer
    {
        T value;
        char bytes[sizeof(T)];
    };

    struct rep_type
    {
        native_type dbh;
        cache_type  cache; // Prepared statements cache

        //
        // Key-Value database traits
        //
        static DEBBY__EXPORT result_status write (
              database::rep_type * rep
            , database::key_type const & key
            , int column_family_index
            , char const * data, std::size_t len);

        template <typename T>
        static typename std::enable_if<std::is_integral<T>::value, result_status>::type
        set (database::rep_type * rep, key_type const & key, T value)
        {
            fixed_packer<T> p;
            p.value = value;
            return write(rep, key, INT_COLUMN_FAMILY_INDEX, p.bytes, sizeof(T));
        }

        template <typename T>
        static typename std::enable_if<std::is_floating_point<T>::value, result_status>::type
        set (database::rep_type * rep, key_type const & key, T value)
        {
            fixed_packer<T> p;
            p.value = value;
            return write(rep, key, FP_COLUMN_FAMILY_INDEX, p.bytes, sizeof(T));
        }
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
        , bool create_if_missing = true);

    static DEBBY__EXPORT rep_type make_kv (pfs::filesystem::path const & path
        , bool create_if_missing = true);
};

}}} // namespace debby::backend::sqlite3

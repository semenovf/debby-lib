////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "statement.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/relational_database.hpp"
#include <string>
#include <unordered_map>
#include <stdexcept>

struct sqlite3;

namespace debby {
namespace sqlite3 {

struct database_traits
{
    using statement_type = statement;
};

class database: public relational_database<database, database_traits>
{
    friend class basic_database<database>;
    friend class relational_database<database, database_traits>;

private:
    using base_class     = relational_database<database, database_traits>;
    using native_type    = struct sqlite3 *;
    using statement_type = database_traits::statement_type;
    using cache_type     = std::unordered_map<std::string, statement_type::native_type>;

private:
    native_type _dbh {nullptr};
    cache_type  _cache; // Prepared statements cache

private:
    auto open (pfs::filesystem::path const & path
        , bool create_if_missing
        , error * err) noexcept -> bool;

    void close () noexcept;

    auto is_opened () const noexcept -> bool
    {
        return _dbh != nullptr;
    }

    auto rows_count_impl (std::string const & table_name) -> std::size_t;

    auto prepare_impl (std::string const & sql
        , bool cache
        , error * err) -> statement_type;

    bool query_impl (std::string const & sql, error * perr);

    std::vector<std::string> tables_impl (std::string const & pattern
        , error * perr);

    /**
     * Removes named @a table or drop all tables if @a table is empty.
     */
    bool remove_impl (std::string const & table, error * perr);
    bool exists_impl (std::string const & name, error * perr);
    bool begin_impl ();
    bool commit_impl ();
    bool rollback_impl ();
    auto clear_impl (std::string const & table_name, error * perr) -> bool;
    auto drop_impl (std::string const & table_name, error * perr) -> bool;

public:
    database () = default;

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
     * @param ec Store error code one of the following:
     *         - errc::database_not_found
     *         - errc::backend_error
     *
     * @throws debby::exception.
     */
    database (pfs::filesystem::path const & path
        , bool create_if_missing = true
        , error * perr = nullptr)
    {
        open(path, create_if_missing, perr);
    }

    ~database ()
    {
        close();
    }

    database (database && other)
    {
        *this = std::move(other);
    }

    database & operator = (database && other)
    {
        close();
        _dbh = other._dbh;
        other._dbh = nullptr;
        return *this;
    }
};

}} // namespace debby::sqlite3

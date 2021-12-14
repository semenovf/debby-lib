////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
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

namespace pfs {
namespace debby {
namespace sqlite3 {

struct database_traits
{
    using statement_type = statement;
};

PFS_DEBBY__EXPORT class database: public relational_database<database, database_traits>
{
    friend class basic_database<database>;
    friend class relational_database<database, database_traits>;

private:
    using base_class     = relational_database<database, database_traits>;
    using native_type    = struct sqlite3 *;
    using statement_type = database_traits::statement_type;
    using cache_type     = std::unordered_map<std::string, statement_type::native_type>;

private:
    static std::string const ERROR_DOMAIN;

private:
    native_type _dbh {nullptr};
    cache_type  _cache; // Prepared statements cache

private:
    // throws bad_alloc, runtime_error
    bool open_impl (filesystem::path const & path, bool create_if_missing);

    void close_impl ();

    bool is_opened_impl () const noexcept
    {
        return _dbh != nullptr;
    }

    // throws sql_error
    statement_type prepare_impl (std::string const & sql, bool cache);

    // throws sql_error
    bool query_impl (std::string const & sql);

    // throws sql_error on prepearing statement failure.
    std::vector<std::string> tables_impl (std::string const & pattern);

    // throws sql_error
    bool clear_impl ();

    // throws sql_error
    bool exists_impl (std::string const & name);

    // throws sql_error
    bool begin_impl ();

    // throws sql_error
    bool commit_impl ();

    // throws sql_error
    bool rollback_impl ();

public:
    database () {}

    ~database ()
    {
        close_impl();
    }

    database (database const &) = delete;
    database & operator = (database const &) = delete;

    database (database && other)
    {
        *this = std::move(other);
    }

    database & operator = (database && other)
    {
        close_impl();
        _dbh = other._dbh;
        other._dbh = nullptr;
        return *this;
    }
};

}}} // namespace pfs::debby::sqlite3

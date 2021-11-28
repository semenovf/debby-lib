////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "sqlite3.h"
#include "statement.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/basic_database.hpp"
#include <string>

namespace pfs {
namespace debby {
namespace sqlite3 {

PFS_DEBBY__EXPORT class database: public basic_database<database>
{
public:
    using statement_type = statement;

private:
    friend class basic_database<database>;

    using base_class = basic_database<database>;
    using handle_type = struct sqlite3 *;

    handle_type _dbh {nullptr};
    std::string _last_error;

private:
    std::string last_error_impl () const noexcept
    {
        return _last_error;
    }

    bool open_impl (filesystem::path const & path);
    void close_impl ();

    bool is_opened_impl () const noexcept
    {
        return _dbh != nullptr;
    }

    bool clear_impl ();
    std::vector<std::string> tables_impl (std::string const & pattern = std::string{});
    bool exists_impl (std::string const & name);
    statement prepare_impl (std::string const & sql);

    bool query_impl (std::string const & sql);

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

    statement_type prepare (std::string const & sql)
    {
        return /*static_cast<Impl*>(this)->*/prepare_impl(sql);
    }
};

}}} // namespace pfs::debby::sqlite3

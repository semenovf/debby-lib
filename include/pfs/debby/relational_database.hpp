////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_database.hpp"
#include <string>
#include <vector>

namespace pfs {
namespace debby {

template <typename Impl, typename Traits>
class relational_database : public basic_database<Impl>
{
    using statement_type = typename Traits::statement_type;

public:
    /**
     * Prepares statement.
     *
     * @throw debby::sql_error on backend failure.
     */
    statement_type prepare (std::string const & sql, bool cache = true)
    {
        return static_cast<Impl*>(this)->prepare_impl(sql, cache);
    }

    /**
     * Executes SQL query.
     *
     * @throw debby::sql_error on backend failure.
     */
    bool query (std::string const & sql)
    {
        return static_cast<Impl *>(this)->query_impl(sql);
    }

    /**
     * Lists available tables at database by pattern.
     *
     * @throw debby::sql_error on backend failure.
     */
    std::vector<std::string> tables (std::string const & pattern = std::string{})
    {
        return static_cast<Impl *>(this)->tables_impl(pattern);
    }

    /**
     * Drops database (delete all tables).
     *
     * @throw debby::sql_error on backend failure.
     */
    bool clear ()
    {
        return static_cast<Impl *>(this)->clear_impl();
    }

    /**
     * @throw debby::sql_error on backend failure.
     */
    bool begin ()
    {
        return static_cast<Impl *>(this)->begin_impl();
    }

    /**
     * @throw debby::sql_error on backend failure.
     */
    bool commit ()
    {
        return static_cast<Impl *>(this)->commit_impl();
    }

    /**
     * @throw debby::sql_error on backend failure.
     */
    bool rollback ()
    {
        return static_cast<Impl *>(this)->rollback_impl();
    }

    /**
     * Checks if named table exists at database.
     *
     * @throw debby::sql_error on backend failure.
     */
    bool exists (std::string const & name)
    {
        return static_cast<Impl *>(this)->exists_impl(name);
    }
};

}} // namespace pfs::debby


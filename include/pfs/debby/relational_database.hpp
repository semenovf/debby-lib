////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.18 Reimplemented with new error handling.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_database.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/optional.hpp"
#include <string>
#include <vector>

namespace debby {

template <typename Impl, typename Traits>
class relational_database : public basic_database<Impl>
{
    using statement_type = typename Traits::statement_type;

public:
    /**
     * Return rows count in named table.
     */
    std::size_t rows_count (std::string const & table_name)
    {
        return static_cast<Impl *>(this)->rows_count_impl(table_name);
    }

    /**
     * Prepares statement.
     */
    statement_type prepare (std::string const & sql, bool cache, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->prepare_impl(sql, cache, perr);
    }

    statement_type prepare (std::string const & sql)
    {
        return static_cast<Impl*>(this)->prepare_impl(sql, true, nullptr);
    }

    /**
     * Executes SQL query.
     */
    bool query (std::string const & sql, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->query_impl(sql, perr);
    }

    /**
     * Lists available tables at database by pattern.
     *
     * @return Tables available according to @a pattern.
     */
    std::vector<std::string> tables (std::string const & pattern = std::string{}
        , error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->tables_impl(pattern, perr);
    }

    /**
     * Clear all records from @a table.
     */
    auto clear (std::string const & table, error * perr = nullptr) -> bool
    {
        return static_cast<Impl *>(this)->clear_impl(table, perr);
    }

    /**
     * Removes named @a table or drop all tables if @a table is empty.
     */
    bool remove (std::string const & table, error * perr = nullptr)
    {
        std::vector<std::string> tables{table};
        return static_cast<Impl *>(this)->remove_impl(tables.cbegin(), tables.cend(), perr);
    }

    /**
     * Removes named @a tables or drop all tables if @a tables is empty.
     */
    bool remove (std::vector<std::string> const & tables, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->remove_impl(tables.cbegin(), tables.cend(), perr);
    }

    /**
     * Drops database (delete all tables).
     */
    bool remove_all (error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->remove_impl(perr);
    }

    /**
     * Begin transaction.
     */
    bool begin ()
    {
        return static_cast<Impl *>(this)->begin_impl();
    }

    /**
     * Commit transaction.
     */
    bool commit ()
    {
        return static_cast<Impl *>(this)->commit_impl();
    }

    /**
     * Rollback transaction.
     */
    bool rollback ()
    {
        return static_cast<Impl *>(this)->rollback_impl();
    }

    /**
     * Checks if named table exists at database.
     */
    bool exists (std::string const & name, error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->exists_impl(name, perr);
    }
};

} // namespace debby

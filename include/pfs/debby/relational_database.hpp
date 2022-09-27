////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "exports.hpp"
#include <memory>
#include <string>
#include <vector>

namespace debby {

template <typename Backend>
class relational_database final
{
    using rep_type = typename Backend::rep_type;

public:
    using statement_type = typename Backend::statement_type;

private:
    rep_type _rep;

private:
    relational_database () = delete;
    DEBBY__EXPORT relational_database (rep_type && rep);
    relational_database (relational_database const & other) = delete;
    relational_database & operator = (relational_database const & other) = delete;
    relational_database & operator = (relational_database && other) = delete;

public:
    DEBBY__EXPORT relational_database (relational_database && other);
    DEBBY__EXPORT ~relational_database ();

public:
    /**
     * Checks if database is open.
     */
    DEBBY__EXPORT operator bool () const noexcept;

    /**
     * Return rows count in named table.
     */
    DEBBY__EXPORT std::size_t rows_count (std::string const & table_name);

    /**
     * Prepares statement.
     */
    DEBBY__EXPORT statement_type prepare (std::string const & sql, bool cache = true);

    /**
     * Executes SQL query.
     *
     * @throw debby::error on error.
     */
    DEBBY__EXPORT void query (std::string const & sql);

    /**
     * Lists available tables at database by pattern.
     *
     * @return Tables available according to @a pattern.
     */
    DEBBY__EXPORT std::vector<std::string> tables (std::string const & pattern = std::string{});

    /**
     * Clear all records from @a table.
     */
    DEBBY__EXPORT void clear (std::string const & table);

    /**
     * Removes named @a table or drop all tables if @a table is empty.
     */
    void remove (std::string const & table)
    {
        std::vector<std::string> tables{table};
        remove(tables);
    }

    /**
     * Removes named @a tables or drop all tables if @a tables is empty.
     */
    DEBBY__EXPORT void remove (std::vector<std::string> const & tables);

    /**
     * Drops database (delete all tables).
     */
    DEBBY__EXPORT void remove_all ();

    /**
     * Begin transaction.
     */
    DEBBY__EXPORT void begin ();

    /**
     * Commit transaction.
     */
    DEBBY__EXPORT void commit ();

    /**
     * Rollback transaction.
     */
    DEBBY__EXPORT void rollback ();

    /**
     * Checks if named table exists in database.
     */
    DEBBY__EXPORT bool exists (std::string const & name);

public:
    /**
     * @throw debby::error on create/open data failure.
     */
    template <typename ...Args>
    static relational_database make (Args &&... args)
    {
        return relational_database{Backend::make(std::forward<Args>(args)...)};
    }

    /**
     * @throw debby::error on create/open data failure.
     */
    template <typename ...Args>
    static std::unique_ptr<relational_database> make_unique (Args &&... args)
    {
        auto ptr = new relational_database {Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<relational_database>(ptr);
    }
};

} // namespace debby

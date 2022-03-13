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
    relational_database (rep_type && rep);
    relational_database (relational_database const & other) = delete;
    relational_database & operator = (relational_database const & other) = delete;
    relational_database & operator = (relational_database && other) = delete;

public:
    relational_database (relational_database && other);
    ~relational_database ();

public:
    /**
     * Checks if database is open.
     */
    operator bool () const noexcept;

    /**
     * Return rows count in named table.
     */
    std::size_t rows_count (std::string const & table_name);

    /**
     * Prepares statement.
     */
    statement_type prepare (std::string const & sql, bool cache = true);

    /**
     * Executes SQL query.
     *
     * @throw debby::error on error.
     */
    void query (std::string const & sql);

    /**
     * Lists available tables at database by pattern.
     *
     * @return Tables available according to @a pattern.
     */
    std::vector<std::string> tables (std::string const & pattern = std::string{});

    /**
     * Clear all records from @a table.
     */
    void clear (std::string const & table);

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
    void remove (std::vector<std::string> const & tables);

    /**
     * Drops database (delete all tables).
     */
    void remove_all ();

    /**
     * Begin transaction.
     */
    void begin ();

    /**
     * Commit transaction.
     */
    void commit ();

    /**
     * Rollback transaction.
     */
    void rollback ();

    /**
     * Checks if named table exists in database.
     */
    bool exists (std::string const & name);

public:
    template <typename ...Args>
    static relational_database make (Args &&... args)
    {
        return relational_database{Backend::make(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<relational_database> make_unique (Args &&... args)
    {
        auto ptr = new relational_database {Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<relational_database>(ptr);
    }
};

} // namespace debby

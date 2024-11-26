////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2024.10.29 V2 started.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "backend_enum.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "namespace.hpp"
#include "result.hpp"
#include "statement.hpp"
#include <pfs/optional.hpp>
#include <memory>
#include <string>
#include <vector>

DEBBY__NAMESPACE_BEGIN

template <backend_enum Backend>
class relational_database
{
public:
    static constexpr backend_enum backend_value = Backend;

public:
    class impl;
    using statement_type = statement<Backend>;
    using result_type = result<Backend>;

private:
    std::unique_ptr<impl> _d;

public:
    DEBBY__EXPORT relational_database ();
    DEBBY__EXPORT relational_database (impl && d) noexcept;
    DEBBY__EXPORT relational_database (relational_database && other) noexcept;
    DEBBY__EXPORT ~relational_database ();
    DEBBY__EXPORT relational_database & operator = (relational_database && other) noexcept;

    relational_database (relational_database const & other) = delete;
    relational_database & operator = (relational_database const & other) = delete;

public:
    /**
     * Checks if database is open.
     */
    inline operator bool () const noexcept
    {
        return _d != nullptr;
    }

    /**
     * Returns rows count in named table.
     */
    DEBBY__EXPORT std::size_t rows_count (std::string const & table_name, error * perr = nullptr);

    /**
     * Prepares statement.
     */
    DEBBY__EXPORT statement_type prepare (std::string const & sql, error * perr = nullptr);

    /**
     * Prepares statement and cache it.
     */
    DEBBY__EXPORT statement_type prepare_cached (std::string const & sql, error * perr = nullptr);

    /**
     * Executes SQL query.
     *
     * @throw debby::error on error.
     */
    DEBBY__EXPORT void query (std::string const & sql, error * perr = nullptr);

    /**
     * Executes SQL query and return result.
     *
     * @throw debby::error on error if @a perr equals to @c nullptr.
     */
    DEBBY__EXPORT result_type exec (std::string const & sql, error * perr = nullptr);

    /**
     * Lists available tables at database by pattern.
     *
     * @return Tables available according to @a pattern.
     */
    DEBBY__EXPORT std::vector<std::string> tables (std::string const & pattern = std::string{}
        , error * perr = nullptr);

    /**
     * Clear all records from @a table.
     */
    DEBBY__EXPORT void clear (std::string const & table, error * perr = nullptr);

    /**
     * Removes named @a table or drop all tables if @a table is empty.
     */
    void remove (std::string const & table, error * perr = nullptr)
    {
        std::vector<std::string> tables{table};
        remove(tables, perr);
    }

    /**
     * Removes named @a tables or drop all tables if @a tables is empty.
     */
    DEBBY__EXPORT void remove (std::vector<std::string> const & tables, error * perr = nullptr);

    /**
     * Drops database (delete all tables).
     */
    DEBBY__EXPORT void remove_all (error * perr = nullptr);

    /**
     * Begin transaction.
     */
    DEBBY__EXPORT void begin (error * perr = nullptr);

    /**
     * Commit transaction.
     */
    DEBBY__EXPORT void commit (error * perr = nullptr);

    /**
     * Rollback transaction.
     */
    DEBBY__EXPORT void rollback (error * perr = nullptr);

    /**
     * Checks if named table exists in database.
     */
    DEBBY__EXPORT bool exists (std::string const & name, error * perr = nullptr);

    /**
     * Do transaction with body representing by @a func().
     *
     * @param func transaction body with signature @c optional().
     * @return @c nullopt on success or @c std::string containing an error description otherwise.
     * @throws @c debby::error on failure of commit() or rollback() calls.
     */
    template <typename TransactionBody>
    pfs::optional<std::string> transaction (TransactionBody && func)
    {
        error err;
        begin(& err);

        if (err)
            return pfs::make_optional(std::string{err.what()});

        auto failure = func();

        if (failure) {
            rollback(); // An exception should be thrown on error (inconsistency may occur)
            return failure;
        } else {
            commit(); // An exception should be thrown on error (inconsistency may occur)
        }

        return pfs::nullopt;
    }

public:
    /**
     * See description for backend specific make() functions.
     */
    template <typename ...Args>
    static relational_database make (Args &&... args);

    /**
     * See description for backend specific wipe() function.
     */
    template <typename ...Args>
    static bool wipe (Args &&... args);
};

DEBBY__NAMESPACE_END

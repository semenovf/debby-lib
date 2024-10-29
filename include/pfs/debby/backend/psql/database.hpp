////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "statement.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/statement.hpp"
#include <unordered_map>

struct pg_conn;

namespace debby {
namespace backend {
namespace psql {

struct database
{
    using native_type    = struct pg_conn *;
    using statement_type = debby::statement<debby::backend::psql::statement>;
    using notice_processor_type = void (*) (void * arg, char const * message);

    // // Key-Value database traits
    // using key_type = std::string;

    struct rep_type
    {
        native_type dbh;
        // std::string kv_table_name; // table name for key-value storage
    };

    /**
     * Connect to the database specified by connection parameters @a conninfo as a relational database.
     *
     * @param conninfo Connection parameters. Can be empty to use all default parameters, or it can
     *        contain one or more parameter settings separated by whitespace, or it can contain a URI.
     *
     * @see https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PARAMKEYWORDS
     */
    static DEBBY__EXPORT rep_type make_r (std::string const & conninfo, error * perr = nullptr);
    static DEBBY__EXPORT rep_type make_r (std::string const & conninfo, notice_processor_type proc
        , error * perr = nullptr);

    template <typename ForwardIt>
    static std::string make_conninfo_helper (ForwardIt first, ForwardIt last)
    {
        std::string conninfo;
        auto pos = first;

        if (pos != last) {
            conninfo += pos->first + "=" + pos->second;
            ++pos;
        }

        for (; pos != last; ++pos)
            conninfo += ' ' + pos->first + "=" + pos->second;

        return conninfo;
    }

    /**
     * Connect to the database specified by connection parameters @a conninfo as a relational database.
     *
     * @param conninfo Connection parameters. Can be empty to use all default parameters, or it can
     *        contain one or more parameter settings separated by whitespace, or it can contain a URI.
     *
     * @see https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PARAMKEYWORDS
     */
    template <typename ForwardIt>
    static rep_type make_r (ForwardIt first, ForwardIt last, error * perr = nullptr)
    {
        auto conninfo = make_conninfo_helper(first, last);
        return make_r(conninfo, perr);
    }

    template <typename ForwardIt>
    static rep_type make_r (ForwardIt first, ForwardIt last, notice_processor_type proc, error * perr = nullptr)
    {
        auto conninfo = make_conninfo_helper(first, last);
        return make_r(conninfo, proc, perr);
    }

    /**
     * Drops specified database.
     */
    static DEBBY__EXPORT bool wipe (std::string const & db_name, std::string const & conninfo
        , error * perr = nullptr);

    /**
     * Drops specified database.
     */
    template <typename ForwardIt>
    static bool wipe (std::string const & db_name, ForwardIt first, ForwardIt last, error * perr = nullptr)
    {
        auto conninfo = make_conninfo_helper(first, last);
        return wipe(db_name, conninfo, perr);
    }

    // /**
    //  * Connect to the database specified by connection parameters @a conninfo as a key-value storage.
    //  *
    //  * @param conninfo Connection parameters. Can be empty to use all default parameters, or it can
    //  *        contain one or more parameter settings separated by whitespace, or it can contain a URI.
    //  */
    // static DEBBY__EXPORT rep_type make_kv (std::string const & table_name, std::string const & conninfo, error * perr = nullptr);
    // static DEBBY__EXPORT rep_type make_kv (std::string const & table_name, std::string const & conninfo, notice_processor_type proc
    //     , error * perr = nullptr);
    //
    // template <typename ForwardIt>
    // static rep_type make_kv (std::string const & table_name, ForwardIt first, ForwardIt last, error * perr = nullptr)
    // {
    //     auto conninfo = make_conninfo_helper(first, last);
    //     return make_kv(table_name, conninfo, perr);
    // }
    //
    // template <typename ForwardIt>
    // static rep_type make_kv (std::string const & table_name, ForwardIt first, ForwardIt last, notice_processor_type proc, error * perr = nullptr)
    // {
    //     auto conninfo = make_conninfo_helper(first, last);
    //     return make_kv(table_name, conninfo, proc, perr);
    // }
};

}}} // namespace debby::backend::psql

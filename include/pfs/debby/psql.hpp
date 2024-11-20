////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.02 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "exports.hpp"
#include "namespace.hpp"
#include "keyvalue_database.hpp"
#include "relational_database.hpp"

DEBBY__NAMESPACE_BEGIN

namespace psql {

using notice_processor_type = void (*) (void * arg, char const * message);

/**
 * Connect to the database specified by connection parameters @a conninfo as a relational database.
 *
 * @param conninfo Connection parameters. Can be empty to use all default parameters, or it can
 *        contain one or more parameter settings separated by whitespace, or it can contain a URI.
 *
 * @see https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-PARAMKEYWORDS
 */
DEBBY__EXPORT
relational_database<backend_enum::psql>
make (std::string const & conninfo, error * perr);

DEBBY__EXPORT
relational_database<backend_enum::psql>
make (std::string const & conninfo, notice_processor_type proc , error * perr = nullptr);

template <typename ForwardIt>
std::string build_conninfo (ForwardIt first, ForwardIt last)
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
inline relational_database<backend_enum::psql>
make (ForwardIt first, ForwardIt last, error * perr = nullptr)
{
    auto conninfo = build_conninfo(first, last);
    return make(conninfo, perr);
}

template <typename ForwardIt>
inline relational_database<backend_enum::psql>
make (ForwardIt first, ForwardIt last, notice_processor_type proc, error * perr = nullptr)
{
    auto conninfo = build_conninfo(first, last);
    return make(conninfo, proc, perr);
}

/**
 * Wipes database (e.g. drops database or removes files associated with database if possible).
 *
 * @return @c true if removing was successful, @c false otherwise.
 */
DEBBY__EXPORT
bool wipe (std::string const & db_name, std::string const & conninfo, error * perr);

DEBBY__EXPORT
keyvalue_database<backend_enum::psql>
make_kv (std::string const & conninfo, std::string const & table_name, error * perr = nullptr);

template <typename ForwardIt>
inline keyvalue_database<backend_enum::psql>
make_kv (ForwardIt first, ForwardIt last, std::string const & table_name, error * perr = nullptr)
{
    auto conninfo = build_conninfo(first, last);
    return make_kv(conninfo, table_name, perr);
}

} // namespace psql

template<>
template <typename ...Args>
inline relational_database<backend_enum::psql>
relational_database<backend_enum::psql>::make (Args &&... args)
{
    return relational_database<backend_enum::psql>{psql::make(std::forward<Args>(args)...)};
}

template<>
template <typename ...Args>
inline bool relational_database<backend_enum::psql>::wipe (Args &&... args)
{
    return psql::wipe(std::forward<Args>(args)...);
}

template<>
template <typename ...Args>
inline keyvalue_database<backend_enum::psql>
keyvalue_database<backend_enum::psql>::make (Args &&... args)
{
    return keyvalue_database<backend_enum::psql>{psql::make_kv(std::forward<Args>(args)...)};
}

DEBBY__NAMESPACE_END

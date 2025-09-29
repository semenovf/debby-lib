////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.20 Initial version.
//      2025.09.29 Changed set/get implementation.
////////////////////////////////////////////////////////////////////////////////
#include "../keyvalue_database_common.hpp"
#include "../keyvalue_relational_database_impl.hpp"
#include "relational_database_impl.hpp"
#include "debby/keyvalue_database.hpp"
#include "debby/psql.hpp"
#include <pfs/fmt.hpp>

DEBBY__NAMESPACE_BEGIN

using keyvalue_database_t = keyvalue_database<backend_enum::psql>;

template<> char const * keyvalue_database_t::impl::REMOVE_SQL = R"(DELETE FROM "{}" WHERE key=$1)";
template<> char const * keyvalue_database_t::impl::PUT_SQL = R"(INSERT INTO "{}" (key, value) VALUES ($1, $2) ON CONFLICT (key) DO UPDATE SET key=$1, value=$2)";
template<> char const * keyvalue_database_t::impl::GET_SQL = R"(SELECT value FROM "{}" WHERE key=$1)";

template keyvalue_database_t::keyvalue_database ();
template keyvalue_database_t::keyvalue_database (impl && d) noexcept;
template keyvalue_database_t::keyvalue_database (keyvalue_database_t && other) noexcept;
template keyvalue_database_t::~keyvalue_database ();
template keyvalue_database_t & keyvalue_database_t::operator = (keyvalue_database && other) noexcept;

template void keyvalue_database_t::clear (error * perr);
template void keyvalue_database_t::remove (key_type const & key, error * perr);
template void keyvalue_database_t::set (key_type const & key, char const * value
    , std::size_t len, error * perr);

#define DEBBY__PSQL_SET(t) \
    template void keyvalue_database<backend_enum::psql>::set<t> (key_type const & key, t value, error * perr);

#define DEBBY__PSQL_GET(t) \
    template t keyvalue_database<backend_enum::psql>::get<t> (key_type const & key, error * perr);

DEBBY__PSQL_SET(bool)
DEBBY__PSQL_SET(char)
DEBBY__PSQL_SET(signed char)
DEBBY__PSQL_SET(unsigned char)
DEBBY__PSQL_SET(short int)
DEBBY__PSQL_SET(unsigned short int)
DEBBY__PSQL_SET(int)
DEBBY__PSQL_SET(unsigned int)
DEBBY__PSQL_SET(long int)
DEBBY__PSQL_SET(unsigned long int)
DEBBY__PSQL_SET(long long int)
DEBBY__PSQL_SET(unsigned long long int)
DEBBY__PSQL_SET(float)
DEBBY__PSQL_SET(double)

DEBBY__PSQL_GET(bool)
DEBBY__PSQL_GET(char)
DEBBY__PSQL_GET(signed char)
DEBBY__PSQL_GET(unsigned char)
DEBBY__PSQL_GET(short int)
DEBBY__PSQL_GET(unsigned short int)
DEBBY__PSQL_GET(int)
DEBBY__PSQL_GET(unsigned int)
DEBBY__PSQL_GET(long int)
DEBBY__PSQL_GET(unsigned long int)
DEBBY__PSQL_GET(long long int)
DEBBY__PSQL_GET(unsigned long long int)
DEBBY__PSQL_GET(float)
DEBBY__PSQL_GET(double)
DEBBY__PSQL_GET(std::string)

namespace psql {

keyvalue_database_t
make_kv (std::string const & conninfo, std::string const & table_name, error * perr)
{
    error err;
    auto db = make(conninfo, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return keyvalue_database_t{};
    }

    std::string sql = fmt::format("CREATE TABLE IF NOT EXISTS \"{}\""
        " (key TEXT NOT NULL UNIQUE, value BYTEA, PRIMARY KEY(key))", table_name);

    db.query(sql, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return keyvalue_database_t{};
    }

    keyvalue_database_t::impl d {std::move(db), std::string{table_name}};
    return keyvalue_database_t{std::move(d)};
}

} // namespace backend::psql

DEBBY__NAMESPACE_END

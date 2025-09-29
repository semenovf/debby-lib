////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.04 Initial version.
//      2025.09.29 Changed set/get implementation.
////////////////////////////////////////////////////////////////////////////////
#include "../keyvalue_database_common.hpp"
#include "../keyvalue_relational_database_impl.hpp"
#include "relational_database_impl.hpp"
#include "debby/sqlite3.hpp"
#include <pfs/fmt.hpp>

DEBBY__NAMESPACE_BEGIN

using keyvalue_database_t = keyvalue_database<backend_enum::sqlite3>;

template<> char const * keyvalue_database_t::impl::REMOVE_SQL = R"(DELETE FROM "{}" WHERE key=?)";
template<> char const * keyvalue_database_t::impl::PUT_SQL = R"(INSERT OR REPLACE INTO "{}" (key, value) VALUES (?, ?))";
template<> char const * keyvalue_database_t::impl::GET_SQL = R"(SELECT value FROM "{}" WHERE key=?)";

template keyvalue_database_t::keyvalue_database ();
template keyvalue_database_t::keyvalue_database (impl && d) noexcept;
template keyvalue_database_t::keyvalue_database (keyvalue_database_t && other) noexcept;
template keyvalue_database_t::~keyvalue_database ();
template keyvalue_database_t & keyvalue_database_t::operator = (keyvalue_database && other) noexcept;

template void keyvalue_database_t::clear (error * perr);
template void keyvalue_database_t::remove (key_type const & key, error * perr);
template void keyvalue_database_t::set (key_type const & key, char const * value
    , std::size_t len, error * perr);

#define DEBBY__SQLITE3_SET(t) \
    template void keyvalue_database<backend_enum::sqlite3>::set<t> (key_type const & key, t value, error * perr);

#define DEBBY__SQLITE3_GET(t) \
    template t keyvalue_database<backend_enum::sqlite3>::get<t> (key_type const & key, error * perr);

DEBBY__SQLITE3_SET(bool)
DEBBY__SQLITE3_SET(char)
DEBBY__SQLITE3_SET(signed char)
DEBBY__SQLITE3_SET(unsigned char)
DEBBY__SQLITE3_SET(short int)
DEBBY__SQLITE3_SET(unsigned short int)
DEBBY__SQLITE3_SET(int)
DEBBY__SQLITE3_SET(unsigned int)
DEBBY__SQLITE3_SET(long int)
DEBBY__SQLITE3_SET(unsigned long int)
DEBBY__SQLITE3_SET(long long int)
DEBBY__SQLITE3_SET(unsigned long long int)
DEBBY__SQLITE3_SET(float)
DEBBY__SQLITE3_SET(double)

DEBBY__SQLITE3_GET(bool)
DEBBY__SQLITE3_GET(char)
DEBBY__SQLITE3_GET(signed char)
DEBBY__SQLITE3_GET(unsigned char)
DEBBY__SQLITE3_GET(short int)
DEBBY__SQLITE3_GET(unsigned short int)
DEBBY__SQLITE3_GET(int)
DEBBY__SQLITE3_GET(unsigned int)
DEBBY__SQLITE3_GET(long int)
DEBBY__SQLITE3_GET(unsigned long int)
DEBBY__SQLITE3_GET(long long int)
DEBBY__SQLITE3_GET(unsigned long long int)
DEBBY__SQLITE3_GET(float)
DEBBY__SQLITE3_GET(double)
DEBBY__SQLITE3_GET(std::string)

namespace sqlite3 {

keyvalue_database<backend_enum::sqlite3>
make_kv (pfs::filesystem::path const & path, std::string const & table_name, bool create_if_missing
    , error * perr)
{
    return make_kv(path, table_name, create_if_missing, preset_enum::DEFAULT_PRESET, perr);
}

keyvalue_database<backend_enum::sqlite3>
make_kv (pfs::filesystem::path const & path, std::string const & table_name, bool create_if_missing
    , preset_enum preset, error * perr)
{
    error err;
    auto db = make(path, create_if_missing, preset, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return keyvalue_database_t{};
    }

    std::string sql = fmt::format("CREATE TABLE IF NOT EXISTS \"{}\""
        " (key TEXT NOT NULL UNIQUE, value BLOB"
        " , PRIMARY KEY(key)) WITHOUT ROWID", table_name);

    db.query(sql, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return keyvalue_database_t{};
    }

    keyvalue_database_t::impl d {std::move(db), std::string{table_name}};
    return keyvalue_database_t{std::move(d)};
}

} // namespace backend::sqlite3

DEBBY__NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "../keyvalue_database_common.hpp"
#include "../keyvalue_database_impl.hpp"
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
template void keyvalue_database_t::set_arithmetic (key_type const & key, std::int64_t value, std::size_t size, error * perr);
template void keyvalue_database_t::set_arithmetic (key_type const & key, double value, std::size_t size, error * perr);
template void keyvalue_database_t::set_chars (key_type const & key, char const * data, std::size_t size, error * perr);
template std::int64_t keyvalue_database_t::get_int64 (key_type const & key, error * perr);
template double keyvalue_database_t::get_double (key_type const & key, error * perr);
template std::string keyvalue_database_t::get_string (key_type const & key, error * perr);

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

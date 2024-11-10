////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "debby/keyvalue_database.hpp"
#include "debby/sqlite3.hpp"
#include "../kv_common.hpp"
#include <pfs/assert.hpp>
#include <pfs/fmt.hpp>

DEBBY__NAMESPACE_BEGIN

using keyvalue_database_t = keyvalue_database<backend_enum::sqlite3>;

template <>
class keyvalue_database_t::impl: public relational_database<backend_enum::sqlite3>
{
private:
    std::string _table_name;

public:
    impl (relational_database<backend_enum::sqlite3> && db, std::string && table_name)
        : relational_database<backend_enum::sqlite3>(std::move(db))
        , _table_name(std::move(table_name))
    {}

public:
    std::string const & table_name () const
    {
        return _table_name;
    }

    /**
     * Removes value for @a key.
     */
    void remove (keyvalue_database_t::key_type const & key, error * perr)
    {
        std::string sql = fmt::format("DELETE FROM '{}' WHERE key='{}'", _table_name, key);
        this->query(sql, perr);
    }

    bool put (keyvalue_database_t::key_type const & key, char const * data, std::size_t len, error * perr)
    {
        // Attempt to write `null` data interpreted as delete operation for key
        if (data == nullptr)
            remove(key, perr);

        error err;
        std::string sql =  fmt::format("INSERT OR REPLACE INTO '{}' (key, value) VALUES (?, ?)", _table_name);
        auto stmt = this->prepare(sql, true, & err);

        if (!err) {
            stmt.bind(0, key.c_str(), key.size(), & err)
                && stmt.bind(1, data, len, & err);

            if (!err)
                stmt.exec(& err);

            if (err) {
                pfs::throw_or(perr, std::move(err));
                return false;
            }
        }

        return true;
    }

    template <typename T>
    T get (std::string const & key, error * perr)
    {
        error err;
        std::string sql = fmt::format("SELECT value FROM '{}' WHERE key=?", _table_name);
        auto stmt = this->prepare(sql, true, & err);

        if (!err) {
            stmt.bind(0, key.c_str(), key.size(), & err);

            if (!err) {
                auto res = stmt.exec(& err);

                if (!err) {
                    if (res.has_more())
                        return res.get<T>(0, & err);

                    err = error {errc::key_not_found, tr::f_("key not found: '{}'", key)};
                }
            }
        }

        pfs::throw_or(perr, std::move(err));
        return T{};
    }
};

template <>
keyvalue_database_t::keyvalue_database () = default;

template <>
keyvalue_database_t::keyvalue_database (impl && d)
{
    if (_d == nullptr) {
        _d = new impl(std::move(d));
    } else {
        *_d = std::move(d);
    }
}

template <>
keyvalue_database_t::keyvalue_database (keyvalue_database_t && other)
{
    if (other._d != nullptr) {
        _d = new impl(std::move(*other._d));
    } else {
        delete _d;
        _d = nullptr;
    }
}

template <>
keyvalue_database_t::~keyvalue_database ()
{
    if (_d != nullptr)
        delete _d;

    _d = nullptr;
}

template <>
keyvalue_database_t & keyvalue_database_t::operator = (keyvalue_database && other)
{
    if (other._d != nullptr) {
        if (_d != nullptr) {
            *_d = std::move(std::move(*other._d));
        } else {
            _d = new impl(std::move(*other._d));
        }
    } else {
        if (_d != nullptr) {
            delete _d;
            _d = nullptr;
        }
    }

    return *this;
}

template <>
void keyvalue_database_t::clear (error * perr)
{
    if (_d != nullptr)
        _d->clear(_d->table_name(), perr);
}

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

    // FIXME
    std::string sql = fmt::format("CREATE TABLE IF NOT EXISTS '{}'"
        " (key TEXT NOT NULL UNIQUE, value BLOB"
        " , PRIMARY KEY(key)) WITHOUT ROWID", table_name);

    // std::string sql = fmt::format("CREATE TABLE IF NOT EXISTS '{}'"
    //     " (key TEXT NOT NULL UNIQUE, value TEXT"
    //     " , PRIMARY KEY(key))", table_name);

    db.query(sql, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return keyvalue_database_t{};
    }

    keyvalue_database_t::impl d {std::move(db), std::string{table_name}};
    return keyvalue_database_t{std::move(d)};
}

} // namespace backend::sqlite3

template <>
void keyvalue_database_t::set_arithmetic (key_type const & key, std::int64_t value, std::size_t size, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");

    char buf[sizeof(fixed_packer<std::int64_t>)];
    auto p = new (buf) fixed_packer<std::int64_t>{};
    p->value = value;
    _d->put(key, buf, size, perr);
}

template <>
void keyvalue_database_t::set_arithmetic (key_type const & key, double value, std::size_t size, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");

    char buf[sizeof(fixed_packer<double>)];
    auto p = new (buf) fixed_packer<double>{};
    p->value = value;
    _d->put(key, buf, sizeof(double), perr);
}

template <>
void keyvalue_database_t::set_chars (key_type const & key, char const * data, std::size_t size, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    _d->put(key, data, size, perr);
}

template <>
void
keyvalue_database_t::remove (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    _d->remove(key, perr);
}

template <>
std::int64_t keyvalue_database_t::get_int64 (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->get<std::int64_t>(key, perr);
}

template <>
double keyvalue_database_t::get_double (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->get<double>(key, perr);
}

template <>
std::string keyvalue_database_t::get_string (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->get<std::string>(key, perr);
}

DEBBY__NAMESPACE_END

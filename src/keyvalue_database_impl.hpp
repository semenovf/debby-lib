////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.20 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "debby/keyvalue_database.hpp"
#include "debby/relational_database.hpp"
#include "fixed_packer.hpp"
#include "debby/namespace.hpp"
#include <pfs/i18n.hpp>

DEBBY__NAMESPACE_BEGIN

template <backend_enum Backend>
class keyvalue_database<Backend>::impl: public relational_database<Backend>
{
private:
    static char const * REMOVE_SQL;
    static char const * PUT_SQL;
    static char const * GET_SQL;

private:
    std::string _table_name;

public:
    impl (relational_database<Backend> && db, std::string && table_name)
        : relational_database<Backend>(std::move(db))
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
    void remove (typename keyvalue_database::key_type const & key, error * perr)
    {
        error err;
        std::string sql = fmt::format(REMOVE_SQL, _table_name);

        auto stmt = this->prepare(sql, & err);

        if (!err) {
            stmt.bind(1, key.c_str(), key.size(), & err);

            if (!err)
                stmt.exec(& err);

            if (err) {
                pfs::throw_or(perr, std::move(err));
                return;
            }
        }
    }

    bool put (typename keyvalue_database::key_type const & key, char const * data, std::size_t len, error * perr)
    {
        // Attempt to write `null` data interpreted as delete operation for key
        if (data == nullptr)
            remove(key, perr);

        error err;
        std::string sql = fmt::format(PUT_SQL, _table_name);
        auto stmt = this->prepare(sql, & err);

        if (!err) {
            stmt.bind(1, key.c_str(), key.size(), & err)
                && stmt.bind(2, data, len, & err);

            if (!err)
                stmt.exec(& err);

            if (!err)
                return true;
        }

        pfs::throw_or(perr, std::move(err));
        return false;
    }

    template <typename T>
    T get (std::string const & key, error * perr)
    {
        error err;
        std::string sql = fmt::format(GET_SQL, _table_name);
        auto stmt = this->prepare(sql, & err);

        if (!err) {
            stmt.bind(1, key.c_str(), key.size(), & err);

            if (!err) {
                auto res = stmt.exec(& err);

                if (!err) {
                    if (res.has_more()) {
                        auto opt = res.template get<T>(0, & err);

                        if (opt) {
                            return *opt;
                        } else {
                            if (!std::is_same<std::string, typename std::decay<T>::type>::value)
                                err = error {make_error_code(errc::bad_value), tr::f_("value is null for key: '{}'", key)};
                        }
                    } else {
                        err = error {make_error_code(errc::key_not_found), tr::f_("key not found: '{}'", key)};
                    }
                }
            }
        }

        if (err)
            pfs::throw_or(perr, std::move(err));

        return T{}; // empty string for T => std::string
    }
};

template <backend_enum Backend>
void keyvalue_database<Backend>::clear (error * perr)
{
    if (_d != nullptr)
        _d->clear(_d->table_name(), perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::remove (key_type const & key, error * perr)
{
    _d->remove(key, perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::set_arithmetic (key_type const & key, std::int64_t value, std::size_t size, error * perr)
{
    char buf[sizeof(fixed_packer<std::int64_t>)];
    auto p = new (buf) fixed_packer<std::int64_t>{};
    p->value = value;
    _d->put(key, buf, sizeof(std::int64_t), perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::set_arithmetic (key_type const & key, double value, std::size_t size, error * perr)
{
    char buf[sizeof(fixed_packer<double>)];
    auto p = new (buf) fixed_packer<double>{};
    p->value = value;
    _d->put(key, buf, sizeof(double), perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::set_chars (key_type const & key, char const * data, std::size_t size, error * perr)
{
    _d->put(key, data, size, perr);
}

template <backend_enum Backend>
std::int64_t keyvalue_database<Backend>::get_int64 (key_type const & key, error * perr)
{
    return _d->template get<std::int64_t>(key, perr);
}

template <backend_enum Backend>
double keyvalue_database<Backend>::get_double (key_type const & key, error * perr)
{
    return _d->template get<double>(key, perr);
}

template <backend_enum Backend>
std::string keyvalue_database<Backend>::get_string (key_type const & key, error * perr)
{
    return _d->template get<std::string>(key, perr);
}

DEBBY__NAMESPACE_END

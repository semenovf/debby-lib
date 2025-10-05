////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.20 Initial version.
//      2025.09.29 Changed set/get implementation.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "debby/keyvalue_database.hpp"
#include "debby/relational_database.hpp"
#include "fixed_packer.hpp"
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
    statement<Backend> _put_stmt;
    mutable statement<Backend> _get_stmt;

public:
    impl (relational_database<Backend> && db, std::string && table_name)
        : relational_database<Backend>(std::move(db))
        , _table_name(std::move(table_name))
    {
        {
            std::string sql = fmt::format(PUT_SQL, _table_name);
            _put_stmt = this->prepare(sql);
        }

        {
            std::string sql = fmt::format(GET_SQL, _table_name);
            _get_stmt = this->prepare(sql);
        }
    }

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
        _put_stmt.reset(& err);

        if (!err) {
            _put_stmt.bind(1, key.c_str(), key.size(), & err)
                && _put_stmt.bind(2, data, len, & err);

            if (!err)
                _put_stmt.exec(& err);

            if (!err)
                return true;
        }

        pfs::throw_or(perr, std::move(err));
        return false;
    }

    template <typename T>
    T get (std::string const & key, error * perr) const
    {
        error err;
        _get_stmt.reset(& err);

        if (!err) {
            _get_stmt.bind(1, key.c_str(), key.size(), & err);

            if (!err) {
                auto res = _get_stmt.exec(& err);

                if (!err) {
                    if (res.has_more()) {
                        auto opt = res.template get<T>(1, & err);

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
    _d->clear(_d->table_name(), perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::remove (key_type const & key, error * perr)
{
    _d->remove(key, perr);
}

template <backend_enum Backend>
void keyvalue_database<Backend>::set (key_type const & key, char const * value, std::size_t len
    , error * perr)
{
    _d->put(key, value, len, perr);
}

template <backend_enum Backend>
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value, void>
keyvalue_database<Backend>::set (key_type const & key, T value, error * perr)
{
    char buf[sizeof(fixed_packer<T>)];
    auto p = new (buf) fixed_packer<T>{};
    p->value = value;
    _d->put(key, buf, sizeof(T), perr);
}

template <backend_enum Backend>
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value || std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>
keyvalue_database<Backend>::get (key_type const & key, error * perr) const
{
    return _d->template get<std::decay_t<T>>(key, perr);
}

DEBBY__NAMESPACE_END

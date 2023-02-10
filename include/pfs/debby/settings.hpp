////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/string_view.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/keyvalue_database.hpp"

namespace debby {

template <typename Backend>
class settings
{
    using storage_type = keyvalue_database<Backend>;

public:
    using key_type = typename storage_type::key_type;

private:
    storage_type _db;

public:
    settings (storage_type && db)
        : _db(std::move(db))
    {}

    operator bool () const noexcept
    {
        return !!_db;
    }

    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, void>::type
    set (key_type const & key, T value, error * perr = nullptr)
    {
        _db.template set<T>(key, value, perr);
    }

    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value, void>::type
    set (key_type const & key, T value, error * perr = nullptr)
    {
        _db.template set<T>(key, value, perr);
    }

    void set (key_type const & key, std::string const & value, error * perr = nullptr)
    {
        _db.set(key, value, perr);
    }

    void set (key_type const & key, pfs::string_view value, error * perr = nullptr)
    {
        _db.set(key, std::move(value), perr);
    }

    void set (key_type const & key, char const * value, error * perr = nullptr)
    {
        _db.set(key, value, perr);
    }

    template <typename T>
    T get (key_type const & key, T const & default_value = T{}, error * perr = nullptr) const
    {
        return _db.template get_or<T>(key, default_value, perr);
    }

    template <typename T>
    T take (key_type const & key, T const & default_value = T{}, error * perr = nullptr)
    {
        error err;
        auto v = _db.template get<T>(key, & err);

        if (err.code() == make_error_code(errc::key_not_found)) {
            err = error {errc::success};
            set(key, default_value, & err);

            if (!err)
                return get(key, default_value, perr);
        }

        if (err) {
            if (perr)
                *perr = err;
            else
                throw err;
        }

        return default_value;
    }

    void remove (key_type const & key, error * perr = nullptr)
    {
        _db.remove(key, perr);
    }

public:
    template <typename ...Args>
    static settings make (Args &&... args)
    {
        return settings {storage_type::make(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<settings> make_unique (Args &&... args)
    {
        auto ptr = new settings {storage_type::make(std::forward<Args>(args)...)};
        return std::unique_ptr<settings>(ptr);
    }
};

} // namespace debby

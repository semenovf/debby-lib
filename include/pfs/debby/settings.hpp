////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.08 Initial version.
//      2024.11.04 V2 started.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "backend_enum.hpp"
#include "keyvalue_database.hpp"
#include "namespace.hpp"
#include <pfs/optional.hpp>
#include <pfs/string_view.hpp>
#include <memory>

namespace debby {

template <backend_enum Backend>
class settings
{
    using storage_type = keyvalue_database<Backend>;

public:
    using key_type = typename storage_type::key_type;
    using string_view = pfs::string_view;

private:
    storage_type _db;

public:
    settings () = default;

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

    template <typename T>
    void set (key_type const & key, pfs::optional<T> const & opt_value, error * perr = nullptr)
    {
        if (opt_value)
            this->set(key, *opt_value, perr);
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
    T get (key_type const & key, T const & default_value = T{}, error * perr = nullptr)
    {
        return _db.template get_or<T>(key, default_value, perr);
    }

    template <typename T>
    T take (key_type const & key, T const & default_value = T{}, error * perr = nullptr)
    {
        error err;
        auto v = _db.template get<T>(key, & err);

        if (!err)
            return v;

        if (err.code() == make_error_code(errc::key_not_found)) {
            err = error {};
            set(key, default_value, & err);

            if (!err)
                return get(key, default_value, perr);
        }

        pfs::throw_or(perr, std::move(err));
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
};

} // namespace debby

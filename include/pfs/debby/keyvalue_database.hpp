////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_database.hpp"
#include "error.hpp"
#include "unified_value.hpp"
#include "pfs/optional.hpp"
#include <string>
#include <system_error>
#include <vector>
#include <cstring>

namespace debby {

template <typename Impl, typename Traits>
class keyvalue_database : public basic_database<Impl>
{
    using key_type = typename Traits::key_type;

public:
    using value_type = unified_value;

protected:
    using basic_database<Impl>::basic_database;

public:
    /**
     * Drops database (delete all tables).
     */
    bool clear (error * perr = nullptr)
    {
        return static_cast<Impl *>(this)->clear_impl(perr);
    }

    /**
     * Stores arithmetic type @a value associated with @a key into database.
     */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, bool>::type
    set (key_type const & key, T value, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->set_impl(key, value, perr);
    }

    /**
     * Stores string @a value associated with @a key into database.
     */
    bool set (key_type const & key, std::string const & value, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->set_impl(key, value, perr);
    }

    /**
     * Stores character sequence @a value with length @a len associated
     * with @a key into database.
     */
    bool set (key_type const & key, char const * value
        , std::size_t len, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->set_impl(key, value, len, perr);
    }

    /**
     * Stores C-string @a value associated with @a key into database.
     */
    bool set (key_type const & key, char const * value, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->set_impl(key, value
            , std::strlen(value), perr);
    }

    template <typename T>
    bool pull (key_type const & key, pfs::optional<T> & target, error * perr = nullptr)
    {
        error err;
        auto value = static_cast<Impl*>(this)->template fetch_impl<T>(key, & err);

        if (err) {
            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        }

        if (pfs::holds_alternative<std::nullptr_t>(value)) {
            target = pfs::nullopt;
            return true;
        }

        auto p = get_if<T>(& value);

        // Bad casting
        if (!p) {
            auto ec = make_error_code(errc::bad_value);
            auto err = error{ec, fmt::format("unsuitable value stored by key: {}"
                , key)};
            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        }

        target = std::move(static_cast<T>(*p));
        return true;
    }

    /**
     * Pulls value for specified column @a name and assigns it to @a target.
     * Column can't be nullable.
     *
     * @param name Column name.
     * @param target Reference to store result.
     * @param perr Pointer to store error or @c nullptr to allow throwing exceptions.
     *
     * @return @c true on success pull, or @c false if otherwise.
     */
    template <typename T>
    bool pull (key_type const & key, T & target, error * perr = nullptr)
    {
        pfs::optional<T> opt;

        if (!pull(key, opt, perr))
            return false;

        // Column can't contains `null` value, use method above.
        if (!opt)
            return false;

        target = *opt;

        return true;
    }

    template <typename T>
    pfs::optional<T> get (key_type const & key, error * perr = nullptr)
    {
        pfs::optional<T> result;

        if (!pull(key, result, perr))
            return pfs::nullopt;

        return std::move(result);
    }

    /**
     * Removes entry associated with @a key from database.
     */
    bool remove (key_type const & key, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->remove_impl(key, perr);
    }
};

} // namespace debby

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

protected:
    using basic_database<Impl>::basic_database;

public:
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

    /**
     * Pulls arithmetic or string type value associated with @a key from database.
     */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value
        || std::is_same<std::string, T>::value, bool>::type
    pull (key_type const & key, pfs::optional<T> & target, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->pull_impl(key, target, perr);
    }

    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value
        || std::is_same<std::string, T>::value, bool>::type
    pull (key_type const & key, T & target, error * perr = nullptr)
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

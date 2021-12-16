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
#include "expected_result.hpp"
#include <string>
#include <system_error>
#include <vector>
#include <cstring>

namespace pfs {
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
    typename std::enable_if<std::is_arithmetic<T>::value, std::error_code>::type
    set (key_type const & key, T value)
    {
        return static_cast<Impl*>(this)->set_impl(key, value);
    }

    /**
     * Stores string @a value associated with @a key into database.
     */
    std::error_code set (key_type const & key, std::string const & value)
    {
        return static_cast<Impl*>(this)->set_impl(key, value);
    }

    /**
     * Stores character sequence @a value with length @a len associated
     * with @a key into database.
     */
    std::error_code set (key_type const & key, char const * value, std::size_t len)
    {
        return static_cast<Impl*>(this)->set_impl(key, value, len);
    }

    /**
     * Stores C-string @a value associated with @a key into database.
     */
    std::error_code set (key_type const & key, char const * value)
    {
        return static_cast<Impl*>(this)->set_impl(key, value, std::strlen(value));
    }

    /**
     * Fetches value associated with @a key from database.
     */
    template <typename T>
    expected_result<optional<T>> get (key_type const & key)
    {
        return static_cast<Impl*>(this)->template get_impl<T>(key);
    }

    /**
     * Removes entry associated with @a key from database.
     */
    bool remove (key_type const & key)
    {
        return static_cast<Impl*>(this)->remove_impl(key);
    }
};

}} // namespace pfs::debby

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_database.hpp"
#include <string>
#include <vector>
#include <cstring>

namespace pfs {
namespace debby {

template <typename Impl, typename Traits>
class keyvalue_database : public basic_database<Impl>
{
    using key_type = typename Traits::key_type;

public:
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, bool>::type
    set (key_type const & key, T value)
    {
        return static_cast<Impl*>(this)->set_impl(key, value);
    }

    bool set (key_type const & key, std::string const & value)
    {
        return static_cast<Impl*>(this)->set_impl(key, value);
    }

    bool set (key_type const & key, char const * value, std::size_t len)
    {
        return static_cast<Impl*>(this)->set_impl(key, value, len);
    }

    bool set (key_type const & key, char const * value)
    {
        return static_cast<Impl*>(this)->set_impl(key, value, std::strlen(value));
    }

    /**
     * Read value by @a key.
     *
     * @return One of this values:
     *      - expected value if @a key contains any valid value;
     *      - unexpected value @c false if value not found by @a key;
     *      - unexpected value @c true if an error occurred while searching
     *        value by @a key.
     */
    template <typename T>
    expected<T, bool> get (key_type const & key)
    {
        return static_cast<Impl*>(this)->template get_impl<T>(key);
    }
};

}} // namespace pfs::debby

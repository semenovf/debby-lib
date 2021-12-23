////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `debby-lib` library.
//
// Changelog:
//      2021.12.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/string_view.hpp"
#include "pfs/type_traits.hpp"
#include "pfs/variant.hpp"
#include <string>
#include <vector>
#include <cstdint>

#include "pfs/fmt.hpp"

namespace debby {

using blob_t  = std::vector<std::uint8_t>;
using basic_value_t = pfs::variant<
      std::nullptr_t
    , std::intmax_t
    , double
    , blob_t        // bytes sequence
    , std::string>; // utf-8 encoded string

struct unified_value: public basic_value_t
{
    using string_view = pfs::string_view;

    unified_value () : basic_value_t(nullptr) {}

    unified_value (std::nullptr_t)
        : basic_value_t(nullptr)
    {}

    /**
     * Construct @c unified_value from any integral type (bool, char, int, etc).
     */
    template <typename T>
    unified_value (T x, typename std::enable_if<std::is_integral<T>::value>::type * = 0)
        : basic_value_t(static_cast<std::intmax_t>(x))
    {}

    /**
     * Construct unified_value from any floatig point type (float and double).
     */
    template <typename T>
    unified_value (T x, typename std::enable_if<std::is_floating_point<T>::value>::type * = 0)
        : basic_value_t(static_cast<double>(x))
    {}

    /**
     * Construct unified_value from string.
     */
    unified_value (std::string const & x)
        : basic_value_t(x)
    {}

    /**
     * Construct unified_value from string.
     */
    unified_value (char const * x)
        : basic_value_t(x ? std::string{x} : std::string{})
    {}

    /**
     * Construct unified_value from character sequence with length @a len.
     */
    unified_value (char const * x, std::size_t len)
        : basic_value_t(x && len > 0 ? std::string{x, len} : std::string{})
    {}

    /**
     * Construct unified_value from string_view.
     */
    unified_value (string_view x)
        : basic_value_t(x.to_string())
    {}

    /**
     * Construct unified_value from blob.
     */
    unified_value (blob_t const & x)
        : basic_value_t(x)
    {}
};

template <typename T>
inline typename std::enable_if<std::is_same<T, std::nullptr_t>::value, T>::type *
get_if (unified_value * u)
{
    return nullptr;
}

template <typename T>
inline typename std::enable_if<std::is_integral<T>::value, std::intmax_t>::type *
get_if (unified_value * u)
{
    return pfs::get_if<std::intmax_t>(u);
}

template <typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, double>::type *
get_if (unified_value * u)
{
    return pfs::get_if<double>(u);
}

template <typename T>
inline typename std::enable_if<std::is_same<T, std::string>::value, std::string>::type *
get_if (unified_value * u)
{
    return pfs::get_if<std::string>(u);
}

template <typename T>
inline typename std::enable_if<std::is_same<T, blob_t>::value, blob_t>::type *
get_if (unified_value * u)
{
    return pfs::get_if<blob_t>(u);
}

} // namespace debby

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once

namespace pfs {
namespace debby {

template <typename Impl>
class basic_statement
{
public:
    using impl_type = Impl;

public:
    std::string last_error () const noexcept
    {
        return static_cast<Impl const *>(this)->last_error_impl();
    }

    void clear ()
    {
        static_cast<Impl*>(this)->clear_impl();
    }

    template <typename T, bool = std::is_arithmetic<T>::value>
    bool bind (std::string const & placeholder, T value)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder, value);
    }

    template <typename T, bool = std::is_same<std::string, T>::value>
    bool bind (std::string const & placeholder, std::string const & value)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder, value);
    }

    template <typename T, bool = std::is_same<char const *, T>::value>
    bool bind (std::string const & placeholder, char const * value)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder, value);
    }
};

}} // namespace pfs::debby


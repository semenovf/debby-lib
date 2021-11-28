////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/optional.hpp"
#include "pfs/variant.hpp"
#include <string>
#include <vector>

namespace pfs {
namespace debby {

template <typename Impl>
class basic_result
{
public:
    using impl_type = Impl;
    using blob_type = std::vector<std::uint8_t>;
    using value_type = variant<std::nullptr_t
        , bool
        , std::intmax_t
        , double
        , std::string
        , blob_type>;

public:
    std::string last_error () const noexcept
    {
        return static_cast<Impl const *>(this)->last_error_impl();
    }

    bool has_more () const
    {
        return static_cast<Impl const *>(this)->has_more_impl();
    }

    bool is_done () const
    {
        return static_cast<Impl const *>(this)->is_done_impl();
    }

    bool is_error () const
    {
        return static_cast<Impl const *>(this)->is_error_impl();
    }

    void next ()
    {
        static_cast<Impl*>(this)->next_impl();
    }

    int column_count () const
    {
        return static_cast<Impl const *>(this)->column_count_impl();
    }

    std::string column_name (int column) const
    {
        return static_cast<Impl const *>(this)->column_name_impl(column);
    }

    optional<value_type> get (int column)
    {
        return static_cast<Impl*>(this)->get_impl(column);
    }

//     optional<value_type> get (std::string const & name)
//     {
//         return static_cast<Impl*>(this)->get_impl(name);
//     }
};

}} // namespace pfs::debby



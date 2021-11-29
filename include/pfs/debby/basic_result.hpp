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
#include <type_traits>
#include <utility>
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
        , std::intmax_t
        , double
        , std::string
        , blob_type>;

public:
    inline std::string last_error () const noexcept
    {
        return static_cast<Impl const *>(this)->last_error_impl();
    }

    bool has_more () const
    {
        return static_cast<Impl const *>(this)->has_more_impl();
    }

    inline bool is_done () const
    {
        return static_cast<Impl const *>(this)->is_done_impl();
    }

    inline bool is_error () const
    {
        return static_cast<Impl const *>(this)->is_error_impl();
    }

    inline void next ()
    {
        static_cast<Impl*>(this)->next_impl();
    }

    inline int column_count () const
    {
        return static_cast<Impl const *>(this)->column_count_impl();
    }

    inline std::string column_name (int column) const
    {
        return static_cast<Impl const *>(this)->column_name_impl(column);
    }

    inline optional<value_type> fetch (int column)
    {
        return static_cast<Impl*>(this)->fetch_impl(column);
    }

    inline optional<value_type> fetch (std::string const & name)
    {
        return static_cast<Impl*>(this)->fetch_impl(name);
    }

    template <typename T>
    inline std::pair<T, bool> get (int column, T const & default_value)
    {
        auto name = column_name(column);

        if (!name.empty())
            return this->get<T>(name, default_value);

        return std::make_pair(default_value, false);
    }

    template <typename T>
    std::pair<typename std::enable_if<std::is_integral<T>::value, T>::type
        , bool> get (std::string const & name, T const & default_value)
    {
        auto opt = fetch(name);

        if (opt.has_value()) {
            auto * p = pfs::get_if<std::intmax_t>(& *opt);
            return p
                ? std::make_pair(static_cast<T>(*p), true)
                : std::make_pair(default_value, false);
        }

        return std::make_pair(default_value, false);
    }

    template <typename T>
    std::pair<typename std::enable_if<std::is_floating_point<T>::value, T>::type
        , bool> get (std::string const & name, T const & default_value)
    {
        auto opt = fetch(name);

        if (opt.has_value()) {
            auto * p = pfs::get_if<double>(& *opt);
            return p
                ? std::make_pair(static_cast<T>(*p), true)
                : std::make_pair(default_value, false);
        }

        return std::make_pair(default_value, false);
    }

    template <typename T>
    std::pair<typename std::enable_if<std::is_same<T, std::string>::value, T>::type
        , bool> get (std::string const & name, T const & default_value)
    {
        auto opt = fetch(name);

        if (opt.has_value()) {
            auto * p = pfs::get_if<std::string>(& *opt);
            return p
                ? std::make_pair(static_cast<T>(*p), true)
                : std::make_pair(default_value, false);
        }

        return std::make_pair(default_value, false);
    }

    template <typename T>
    std::pair<typename std::enable_if<std::is_same<T, blob_type>::value, T>::type
        , bool> get (std::string const & name, T const & default_value)
    {
        auto opt = fetch(name);

        if (opt.has_value()) {
            auto * p = pfs::get_if<blob_type>(& *opt);
            return p
                ? std::make_pair(static_cast<T>(*p), true)
                : std::make_pair(default_value, false);
        }

        return std::make_pair(default_value, false);
    }
};

}} // namespace pfs::debby



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
#include "pfs/expected.hpp"
#include "pfs/string_view.hpp"
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

    template <typename T, typename E>
    using expected_type = expected<T, E>;

private:
    template <typename T>
    inline typename std::enable_if<std::is_same<T, std::nullptr_t>::value, T>::type *
    get_if_value_pointer (value_type *)
    {
        return nullptr;
    }

    template <typename T>
    inline typename std::enable_if<std::is_integral<T>::value, std::intmax_t>::type *
    get_if_value_pointer (value_type * v)
    {
        return pfs::get_if<std::intmax_t>(v);
    }

    template <typename T>
    inline typename std::enable_if<std::is_floating_point<T>::value, double>::type *
    get_if_value_pointer (value_type * v)
    {
        return pfs::get_if<double>(v);
    }

    template <typename T>
    inline typename std::enable_if<std::is_same<T, std::string>::value, T>::type *
    get_if_value_pointer (value_type * v)
    {
        return pfs::get_if<std::string>(v);
    }

    template <typename T>
    inline typename std::enable_if<std::is_same<T, blob_type>::value, T>::type *
    get_if_value_pointer (value_type * v)
    {
        return pfs::get_if<blob_type>(v);
    }

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

    inline optional<value_type> fetch (string_view const & name)
    {
        return static_cast<Impl*>(this)->fetch_impl(name);
    }

    inline optional<value_type> fetch (std::string const & name)
    {
        return static_cast<Impl*>(this)->fetch_impl(string_view{name});
    }

    /**
     * Get value for specified column @a name.
     *
     * @return One of this values:
     *      - expected value if column @a column contains any valid value;
     *      - unexpected value @c false if column @a column contains @c null value;
     *      - unexpected value @c true if column @a column is out of bounds
     *        or contains value of unexpected type.
     */
    template <typename T>
    expected_type<T, bool> get (string_view name)
    {
        auto opt = fetch(name);

        if (opt.has_value()) {
            if (!holds_alternative<std::nullptr_t>(*opt)) {
                auto p = get_if_value_pointer<T>(& *opt);
                return std::move(static_cast<T>(*p));
            } else {
                return make_unexpected(false);
            }
        }

        return make_unexpected(true);
    }

    template <typename T>
    expected_type<T, bool> get (std::string const & name)
    {
        return this->get<T>(string_view{name});
    }

    /**
     * Get value for specified @a column.
     *
     * @return see @link get(std::string const &) @endlink
     */
    template <typename T>
    inline expected_type<T, bool> get (int column)
    {
        auto name = column_name(column);

        if (!name.empty())
            return this->get<T>(name);

        return make_unexpected(true);
    }
};

}} // namespace pfs::debby

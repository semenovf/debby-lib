////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "pfs/optional.hpp"
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

protected:
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

protected:
    basic_result () = default;

public:
    bool has_more () const noexcept
    {
        return static_cast<Impl const *>(this)->has_more_impl();
    }

    inline bool is_done () const noexcept
    {
        return static_cast<Impl const *>(this)->is_done_impl();
    }

    inline bool is_error () const noexcept
    {
        return static_cast<Impl const *>(this)->is_error_impl();
    }

    /**
     * Steps to next record.
     *
     * @throw sql_error on backend error.
     */
    inline void next ()
    {
        static_cast<Impl*>(this)->next_impl();
    }

    /**
     * Returns column count for this result.
     */
    inline int column_count () const noexcept
    {
        return static_cast<Impl const *>(this)->column_count_impl();
    }

    /**
     * Return column name for @a column.
     */
    inline string_view column_name (int column) const noexcept
    {
        return static_cast<Impl const *>(this)->column_name_impl(column);
    }

    /**
     * Fetches data for specified @a column
     *
     * @throw invalid_argument if column is out of bounds.
     */
    inline value_type fetch (int column)
    {
        return static_cast<Impl*>(this)->fetch_impl(column);
    }

    /**
     * Fetches data for specified column @a name
     *
     * @throw invalid_argument if column @a name is invalid.
     */
    inline value_type fetch (string_view name)
    {
        return static_cast<Impl*>(this)->fetch_impl(name);
    }

    /**
     * Get value for specified column @a name.
     *
     * @return Requested value (inside @c optional) on success fetching and
     *         casting to specified type,
     *         or @c nullopt if column specified by @a name contains @c null value.
     *
     * @throw invalid_argument if column @a name is invalid.
     * @throws bad_cast if unable to cast to requested type.
     */
    template <typename T>
    inline optional<T> get (string_view name)
    {
        return static_cast<Impl*>(this)->template get_impl<T>(name);
    }

    /**
     * Get value for specified @a column.
     *
     * @return see @link get(string_view) @endlink
     *
     * @throws invalid_argument if column is out of bounds.
     */
    template <typename T>
    inline optional<T> get (int column)
    {
        return static_cast<Impl*>(this)->template get_impl<T>(column);
    }
};

}} // namespace pfs::debby

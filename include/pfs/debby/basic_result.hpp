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
#include "pfs/fmt.hpp"
#include "pfs/optional.hpp"
#include "pfs/string_view.hpp"
#include "pfs/variant.hpp"
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace debby {

template <typename Impl>
class basic_result
{
public:
    using impl_type = Impl;
    using blob_type = std::vector<std::uint8_t>;
    using value_type = pfs::variant<std::nullptr_t
        , std::intmax_t
        , double
        , std::string
        , blob_type>;

protected:
    basic_result () = default;
    ~basic_result () = default;
    basic_result (basic_result const &) = delete;
    basic_result & operator = (basic_result const &) = delete;
    basic_result (basic_result && other) = default;
    basic_result & operator = (basic_result && other) = default;

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

public:
    operator bool () const noexcept
    {
        return static_cast<Impl const*>(this)->is_ready();
    }

    bool has_more () const noexcept
    {
        return static_cast<Impl const *>(this)->has_more_impl();
    }

    bool is_done () const noexcept
    {
        return static_cast<Impl const *>(this)->is_done_impl();
    }

    bool is_error () const noexcept
    {
        return static_cast<Impl const *>(this)->is_error_impl();
    }

    /**
     * Returns column count for this result.
     */
    int column_count () const noexcept
    {
        return static_cast<Impl const *>(this)->column_count_impl();
    }

    /**
     * Return column name for @a column.
     */
    pfs::string_view column_name (int column) const noexcept
    {
        return static_cast<Impl const *>(this)->column_name_impl(column);
    }

    /**
     * Steps to next record.
     */
    void next (error * perr = nullptr)
    {
        static_cast<Impl*>(this)->next_impl(perr);
    }

    /**
     * Fetches data for specified @a column
     *
     * @return Value associated with @a column or @c invalid_value if column is
     *         out of bounds.
     */
    value_type fetch (int column, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->fetch_impl(column, perr);
    }

    /**
     * Fetches data for column associated with @a name.
     *
     * @return Value associated with column @a name or @c nullopt if column is
     *         name is bad.
     */
    value_type fetch (pfs::string_view name, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->fetch_impl(name, perr);
    }

    /**
     * Pulls value for specified column @a name and assigns it to @a target.
     * Column can be nullable.
     *
     * @param name Column name.
     * @param target Reference to store result.
     * @param perr Pointer to store error or @c nullptr to allow throwing exceptions.
     *
     * @return @c true on success pull, or @c false if otherwise.
     */
    template <typename T>
    bool pull (pfs::string_view name, pfs::optional<T> & target, error * perr = nullptr)
    {
        error err;
        auto value = fetch(name, & err);

        if (err) {
            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        }

        if (pfs::holds_alternative<std::nullptr_t>(value)) {
            target = pfs::nullopt;
            return true;
        }

        auto p = get_if_value_pointer<T>(& value);

        // Bad casting
        if (!p) {
            auto ec = make_error_code(errc::bad_value);
            auto err = error{ec, fmt::format("unsuitable value stored at column: {}"
                , name.to_string())};
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
    bool pull (pfs::string_view name, T & target, error * perr = nullptr)
    {
        pfs::optional<T> opt;

        if (!pull(name, opt, perr))
            return false;

        // Column can't contains `null` value, use method above.
        if (!opt)
            return false;

        target = *opt;

        return true;
    }

    /**
     * Get value for specified @a column.
     *
     * @see @link get(string_view, pfs::optional<T> &, error *) @endlink
     */
    template <typename T>
    bool pull (int column, pfs::optional<T> & target, error * perr = nullptr)
    {
        return pull(static_cast<Impl*>(this)->column_name_impl(column)
            , target, perr);
    }

    /**
     * Get value for specified @a column.
     *
     * @see @link get(string_view, T &, error *) @endlink
     */
    template <typename T>
    bool pull (int column, T & target, error * perr = nullptr)
    {
        return pull(static_cast<Impl*>(this)->column_name_impl(column)
            , target, perr);
    }

    template <typename T>
    pfs::optional<T> get (pfs::string_view name, error * perr = nullptr)
    {
        pfs::optional<T> result;

        if (!pull(name, result, perr))
            return pfs::nullopt;

        return std::move(result);
    }

    template <typename T>
    pfs::optional<T> get (int column, error * perr = nullptr)
    {
        pfs::optional<T> result;

        if (!pull(column, result, perr))
            return pfs::nullopt;

        return std::move(result);
    }
};

} // namespace debby

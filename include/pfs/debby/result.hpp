////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.26 Initial version.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "unified_value.hpp"
#include "pfs/optional.hpp"
#include "pfs/type_traits.hpp"
#include <string>

namespace debby {

template <typename Backend>
class result final
{
    using rep_type = typename Backend::rep_type;

public:
    using value_type = unified_value;

    class column_wrapper
    {
        value_type _v;

    public:
        column_wrapper (value_type && v) : _v(std::move(v)) {}

        template <typename NativeType>
        void operator >> (NativeType & target)
        {
            Backend::template assign(target, _v);
        }

        template <typename NativeType>
        void operator >> (pfs::optional<NativeType> & target)
        {
            Backend::template assign(target, _v);
        }
    };

private:
    rep_type _rep;

private:
    result () = delete;
    result (rep_type && rep);
    result (result const & other) = delete;
    result & operator = (result const & other) = delete;
    result & operator = (result && other) = delete;

public:
    result (result && other);
    ~result ();

private:
    result_status fetch (int column, value_type & value) const noexcept;
    result_status fetch (std::string const & column_name, value_type & value) const noexcept;

    template <typename T, typename ColumntType>
    typename std::enable_if<!std::is_pointer<T>::value, T>::type
    get_helper (ColumntType column)
    {
        value_type value;
        auto res = fetch(column, value);

        if (!res.ok())
            DEBBY__THROW(res);

        auto ptr = get_if<T>(& value);

        if (!ptr) {
            error err {
                  make_error_code(errc::bad_value)
                , fmt::format("unsuitable data stored in column: {}", column)
            };

            DEBBY__THROW(err);
        }

        return std::move(*ptr);
    }

    template <typename T, typename ColumntType>
    typename std::enable_if<std::is_pointer<T>::value, T>::type
    get_helper (ColumntType column)
    {
        value_type value;
        auto res = fetch(column, value);

        if (!res.ok())
            DEBBY__THROW(res);

        auto ptr = reinterpret_cast<T>(get_if<typename std::remove_pointer<T>::type>(& value));
        return ptr;
    }

    template <typename T, typename ColumntType>
    typename std::enable_if<!std::is_pointer<T>::value, T>::type
    get_or_helper (ColumntType column, T const & default_value)
    {
        value_type value;
        auto res = fetch(column, value);

        if (!res.ok()) {
            if (res.code().value() == static_cast<int>(errc::column_not_found))
                return default_value;

            DEBBY__THROW(res);
        }

        auto ptr = get_if<T>(& value);

        if (!ptr)
            return default_value;

        return std::move(*ptr);
    }

    template <typename T, typename ColumntType>
    typename std::enable_if<std::is_pointer<T>::value, T>::type
    get_or_helper (ColumntType column, T const & default_value)
    {
        value_type value;
        auto res = fetch(column, value);

        if (!res.ok()) {
            if (res.code().value() == static_cast<int>(errc::column_not_found))
                return default_value;

            DEBBY__THROW(res);
        }

        auto ptr = reinterpret_cast<T>(get_if<typename std::remove_pointer<T>::type>(& value));
        return ptr;
    }

public:
    operator bool () const noexcept;

    /**
     */
    bool has_more () const noexcept;

    /**
     */
    bool is_done () const noexcept;

    /**
     */
    bool is_error () const noexcept;

    /**
     * Returns column count for this result.
     */
    int column_count () const noexcept;

    /**
     * Return column name for @a column.
     */
    std::string column_name (int column) const noexcept;

    /**
     * Steps to next record.
     */
    void next ();

    /**
     */
    template <typename T>
    T get (int column)
    {
        return get_helper<T, int>(column);
    }

    /**
     */
    template <typename T>
    T get (std::string const & column)
    {
        return get_helper<T, std::string const &>(column);
    }

    /**
     */
    template <typename T>
    T get_or (int column, T const & default_value)
    {
        return get_or_helper<T, int>(column, default_value);
    }

    /**
     */
    template <typename T>
    T get_or (std::string const & column, T const & default_value)
    {
        return get_or_helper<T, std::string const &>(column, default_value);
    }

    /**
     *
     */
    column_wrapper operator [] (std::string const & column_name)
    {
        value_type v;
        auto res = fetch(column_name, v);

        if (!res.ok())
            DEBBY__THROW(res);

        return column_wrapper(std::move(v));
    }

public:
    template <typename ...Args>
    static result make (Args &&... args)
    {
        return result{Backend::make(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<result> make_unique (Args &&... args)
    {
        auto ptr = new result {Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<result>(ptr);
    }
};

} // namespace debby

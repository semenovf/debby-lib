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
#include "exports.hpp"
#include "unified_value.hpp"
#include "pfs/fmt.hpp"
#include "pfs/optional.hpp"
#include "pfs/type_traits.hpp"
#include <string>

namespace debby {

template <typename Backend>
class result
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
    DEBBY__EXPORT result (rep_type && rep);

public:
    result () = delete;
    result (result const & other) = delete;
    result & operator = (result const & other) = delete;
    result & operator = (result && other) = delete;

    DEBBY__EXPORT result (result && other);
    DEBBY__EXPORT ~result ();

private:
    DEBBY__EXPORT bool fetch (int column, value_type & value, error & perr) const noexcept;
    DEBBY__EXPORT bool fetch (std::string const & column_name, value_type & value, error & perr) const noexcept;

    template <typename T, typename ColumntType>
    typename std::enable_if<!std::is_pointer<T>::value, T>::type
    get_helper (ColumntType column, error * perr = nullptr)
    {
        // Assign type to value
        value_type value = value_type::make_zero<T>();

        error err;
        auto success = fetch(column, value, err);

        if (!success) {
            if (perr) {
                *perr = std::move(err);
                return T{};
            } else {
                throw err;
            }
        }

        auto ptr = get_if<T>(& value);

        if (!ptr) {
            error err {
                  errc::bad_value
                , fmt::format("unsuitable data stored in column: {}", column)
            };

            if (perr) {
                *perr = std::move(err);
                return T{};
            } else {
                throw err;
            }
        }

        return static_cast<T>(*ptr);
    }

    template <typename T, typename ColumntType>
    typename std::enable_if<std::is_pointer<T>::value, T>::type
    get_helper (ColumntType column, error * perr = nullptr)
    {
        value_type value;

        error err;
        auto success = fetch(column, value, err);

        if (!success) {
            if (perr) {
                *perr = std::move(err);
                return T{};
            } else {
                throw err;
            }
        }

        auto ptr = reinterpret_cast<T>(get_if<typename std::remove_pointer<T>::type>(& value));
        return ptr;
    }

    template <typename T, typename ColumntType>
    typename std::enable_if<!std::is_pointer<T>::value, T>::type
    get_or_helper (ColumntType column, T const & default_value, error * perr = nullptr)
    {
        // Assign type to value
        value_type value = value_type::make_zero<T>();

        error err;
        auto success = fetch(column, value, err);

        if (!success) {
            if (err.code().value() == static_cast<int>(errc::column_not_found))
                return default_value;

            if (perr) {
                *perr = std::move(err);
                return T{};
            } else {
                throw err;
            }
        }

        auto ptr = get_if<T>(& value);

        if (!ptr)
            return default_value;

        return static_cast<T>(*ptr);
    }

    template <typename T, typename ColumntType>
    typename std::enable_if<std::is_pointer<T>::value, T>::type
    get_or_helper (ColumntType column, T const & default_value, error * perr = nullptr)
    {
        // Assign type to value
        value_type value = value_type::make_zero<typename std::remove_pointer<T>::type>();

        error err;
        auto success = fetch(column, value, err);

        if (!success) {
            if (err.code().value() == static_cast<int>(errc::column_not_found))
                return default_value;

            if (perr) {
                *perr = std::move(err);
                return T{};
            } else {
                throw err;
            }
        }

        auto ptr = reinterpret_cast<T>(get_if<typename std::remove_pointer<T>::type>(& value));
        return ptr;
    }

public:
    DEBBY__EXPORT operator bool () const noexcept;

    /**
     */
    DEBBY__EXPORT bool has_more () const noexcept;

    /**
     */
    DEBBY__EXPORT bool is_done () const noexcept;

    /**
     */
    //bool is_error () const noexcept;

    /**
     * Returns column count for this result.
     */
    DEBBY__EXPORT int column_count () const noexcept;

    /**
     * Return column name for @a column.
     */
    DEBBY__EXPORT std::string column_name (int column) const noexcept;

    /**
     * Steps to next record.
     */
    DEBBY__EXPORT void next ();

    /**
     */
    template <typename T>
    T get (int column, error * perr = nullptr)
    {
        return get_helper<T, int>(column, perr);
    }

    /**
     */
    template <typename T>
    T get (std::string const & column, error * perr = nullptr)
    {
        return get_helper<T, std::string const &>(column, perr);
    }

    /**
     */
    template <typename T>
    T get_or (int column, T const & default_value, error * perr = nullptr)
    {
        return get_or_helper<T, int>(column, default_value, perr);
    }

    /**
     */
    template <typename T>
    T get_or (std::string const & column, T const & default_value, error * perr = nullptr)
    {
        return get_or_helper<T, std::string const &>(column, default_value, perr);
    }

    /**
     *
     */
    column_wrapper operator [] (std::string const & column_name)
    {
        value_type v;
        error err;
        auto success = fetch(column_name, v, err);

        if (!success)
            throw err;

        return column_wrapper(std::move(v));
    }

    column_wrapper operator [] (char const * column_name)
    {
        return operator [] (std::string{column_name});
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

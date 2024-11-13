////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.26 Initial version.
//      2022.03.12 Refactored.
//      2024.10.29 V2 started.
//      2024.10.30 Fixed API.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "backend_enum.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "namespace.hpp"
#include <pfs/i18n.hpp>
#include <cstdint>
#include <string>

DEBBY__NAMESPACE_BEGIN

template <typename T, typename U>
struct bounded_type;

template <typename T>
struct bounded_type<T, typename std::enable_if<std::is_integral<T>::value, void>::type>
{
    using type = std::intmax_t;
};

template <typename T>
struct bounded_type<T, typename std::enable_if<std::is_floating_point<T>::value, void>::type>
{
    using type = double;
};

template <backend_enum Backend>
class result
{
public:
    class impl;

private:
    impl * _d {nullptr};

public:
    DEBBY__EXPORT result ();
    DEBBY__EXPORT result (impl && d);
    DEBBY__EXPORT result (result && other) noexcept;
    DEBBY__EXPORT ~result ();

    result (result const & other) = delete;
    result & operator = (result const & other) = delete;
    result & operator = (result && other) = delete;

private:
    DEBBY__EXPORT
    std::pair<char *, std::size_t>
    fetch (int column, char * buffer, std::size_t initial_size, error & err) const;

    DEBBY__EXPORT
    std::pair<char *, std::size_t>
    fetch (std::string const & column_name, char * buffer, std::size_t initial_size, error & err) const;

    template <typename T, typename ColumnType>
    typename std::enable_if<std::is_arithmetic<T>::value, bool>::type
    get_helper (ColumnType column, T & result, error * perr = nullptr)
    {
        error err;
        union {
            typename bounded_type<T, void>::type n;
            char buffer [sizeof(typename bounded_type<T, void>::type)];
        } u;

        auto res = fetch(column, u.buffer, sizeof(u.buffer), err);

        if (err) {
            pfs::throw_or(perr, std::move(err));
            return false;
        }

        // Result contains null value
        if (res.first == nullptr)
            return false;

        if (u.buffer != res.first) {
            delete [] res.first;

            pfs::throw_or(perr, error {
                  errc::bad_value
                , tr::f_("unsuitable data stored in column: {}", column)
            });

            return false;
        }

        result = static_cast<T>(u.n);
        return true;
    }

    template <typename /*T*/, typename ColumnType>
    bool
    get_helper (ColumnType column, std::string & result, error * perr = nullptr)
    {
        error err;
        char buffer [64];

        auto res = fetch(column, buffer, sizeof(buffer), err);

        if (err) {
            pfs::throw_or(perr, std::move(err));
            return false;
        }

        // Result contains null value
        if (res.first == nullptr)
            return false;

        result = std::string(res.first, res.second);

        if (buffer != res.first)
            delete [] res.first;

        return true;
    }

public:
    inline operator bool () const noexcept
    {
        return _d != nullptr;
    }

    /**
     */
    DEBBY__EXPORT int rows_affected () const;

    /**
     */
    DEBBY__EXPORT bool has_more () const noexcept;

    /**
     */
    DEBBY__EXPORT bool is_done () const noexcept;

    /**
     */
    DEBBY__EXPORT bool is_error () const noexcept;

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
    template <typename T, typename ColumnType>
    T get (ColumnType column, error * perr = nullptr)
    {
        T result;
        error err;
        auto success = get_helper<T>(column, result, & err);

        if (err) {
            pfs::throw_or(perr, std::move(err));
            return T{};
        }

        if (!success) {
            pfs::throw_or(perr, error{
                  errc::bad_value
                , tr::f_("result is null for column: {}", column)
            });
            return T{};
        }

        return result;
    }

    template <typename T, typename ColumnType>
    T get_or (ColumnType column, T const & default_value, error * perr = nullptr)
    {
        T result;
        error err;
        auto success = get_helper<T>(column, result, & err);

        if (err) {
            pfs::throw_or(perr, std::move(err));
            return T{};
        }

        if (!success)
            return default_value;

        return result;
    }
};

DEBBY__NAMESPACE_END

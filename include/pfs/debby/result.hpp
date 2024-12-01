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
#include <pfs/endian.hpp>
#include <pfs/i18n.hpp>
#include <pfs/numeric_cast.hpp>
#include <pfs/optional.hpp>
#include <pfs/universal_id.hpp>
#include <pfs/time_point.hpp>
#include <cstdint>
#include <type_traits>
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
    DEBBY__EXPORT result (impl && d) noexcept;
    DEBBY__EXPORT result (result && other) noexcept;
    DEBBY__EXPORT ~result ();

    result (result const & other) = delete;
    result & operator = (result const & other) = delete;
    result & operator = (result && other) = delete;

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

    DEBBY__EXPORT pfs::optional<std::int64_t> get_int64 (int column, error * perr = nullptr);
    DEBBY__EXPORT pfs::optional<double> get_double (int column, error * perr = nullptr);
    DEBBY__EXPORT pfs::optional<std::string> get_string (int column, error * perr = nullptr);

    DEBBY__EXPORT pfs::optional<std::int64_t> get_int64 (std::string const & column_name, error * perr = nullptr);
    DEBBY__EXPORT pfs::optional<double> get_double (std::string const & column_name, error * perr = nullptr);
    DEBBY__EXPORT pfs::optional<std::string> get_string (std::string const & column_name, error * perr = nullptr);

    template <typename T, typename ColumnType>
    inline pfs::optional<typename std::enable_if<std::is_integral<T>::value, T>::type>
    get (ColumnType const & column, error * perr = nullptr)
    {
        auto opt = get_int64(column, perr);
        return opt ? pfs::make_optional(pfs::numeric_cast<T>(*opt)) : pfs::nullopt;
    }

    template <typename T, typename ColumnType>
    inline pfs::optional<typename std::enable_if<std::is_floating_point<T>::value, T>::type>
    get (ColumnType const & column, error * perr = nullptr)
    {
        auto opt = get_double(column, perr);
        return opt ? pfs::make_optional(static_cast<T>(*opt)) : pfs::nullopt;
    }

    template <typename T, typename ColumnType>
    inline pfs::optional<typename std::enable_if<std::is_same<std::decay_t<T>, std::string>::value, T>::type>
    get (ColumnType const & column, error * perr = nullptr)
    {
        auto opt = get_string(column, perr);
        return opt ? pfs::make_optional(*opt) : pfs::nullopt;
    }

    template <typename T, typename ColumnType>
    inline pfs::optional<typename std::enable_if<std::is_enum<T>::value, T>::type>
    get (ColumnType const & column, error * perr = nullptr)
    {
        auto opt = get_int64(column, perr);

        if (opt) {
            auto x = pfs::numeric_cast<typename std::underlying_type<T>::type>(*opt);
            return static_cast<T>(x);
        }

        return pfs::nullopt;
    }

    template <typename T, typename ColumnType>
    inline pfs::optional<typename std::enable_if<std::is_same<std::decay_t<T>, pfs::universal_id>::value, T>::type>
    get (ColumnType const & column, error * perr = nullptr)
    {
        auto opt = get_string(column, perr);

        if (opt) {
            auto id = pfs::from_string<pfs::universal_id>(*opt);

            if (id != pfs::universal_id{})
                return pfs::make_optional(id);
        }

        return pfs::nullopt;
    }

    template <typename T, typename ColumnType>
    inline pfs::optional<typename std::enable_if<std::is_same<std::decay_t<T>, pfs::utc_time>::value, T>::type>
    get (ColumnType const & column, error * perr = nullptr)
    {
        auto opt = get_int64(column, perr);

        if (opt)
            return pfs::utc_time{std::chrono::milliseconds{*opt}};

        return pfs::nullopt;
    }

    template <typename T, typename ColumnType>
    inline pfs::optional<typename std::enable_if<std::is_same<std::decay_t<T>, pfs::local_time>::value, T>::type>
    get (ColumnType const & column, error * perr = nullptr)
    {
        auto opt = get_int64(column, perr);

        if (opt)
            return pfs::local_time{std::chrono::milliseconds{*opt}};

        return pfs::nullopt;
    }

    template <typename T, typename ColumnType>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    {
        auto opt = get_int64(column, perr);
        return opt ? pfs::numeric_cast<T>(*opt) : default_value;
    }

    template <typename T, typename ColumnType>
    inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
    get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    {
        auto opt = get_double(column, perr);
        return opt ? static_cast<T>(*opt) : default_value;
    }

    template <typename T, typename ColumnType>
    inline typename std::enable_if<std::is_same<std::decay_t<T>, std::string>::value, T>::type
    get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    {
        auto opt = get_string(column, perr);
        return opt ? *opt : default_value;
    }

    template <typename T, typename ColumnType>
    inline typename std::enable_if<std::is_enum<T>::value, T>::type
    get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    {
        auto opt = get<T>(column, perr);
        return opt ? *opt : default_value;
    }

    template <typename T, typename ColumnType>
    inline typename std::enable_if<std::is_same<std::decay_t<T>, pfs::universal_id>::value, T>::type
    get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    {
        auto opt = get<pfs::universal_id>(column, perr);
        return opt ? *opt : default_value;
    }

    template <typename T, typename ColumnType>
    inline typename std::enable_if<std::is_same<std::decay_t<T>, pfs::utc_time>::value, T>::type
    get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    {
        auto opt = get<pfs::utc_time>(column, perr);
        return opt ? *opt : default_value;
    }

    template <typename T, typename ColumnType>
    inline typename std::enable_if<std::is_same<std::decay_t<T>, pfs::local_time>::value, T>::type
    get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    {
        auto opt = get<pfs::local_time>(column, perr);
        return opt ? *opt : default_value;
    }
};

DEBBY__NAMESPACE_END

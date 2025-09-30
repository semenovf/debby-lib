////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.26 Initial version.
//      2022.03.12 Refactored.
//      2024.10.29 V2 started.
//      2024.10.30 Fixed API.
//      2025.09.30 Changed get implementation.
//                 Added support for custom types.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "namespace.hpp"
#include "backend_enum.hpp"
#include "error.hpp"
#include "exports.hpp"
#include <pfs/endian.hpp>
#include <pfs/i18n.hpp>
#include <pfs/optional.hpp>
#include <cstdint>
#include <type_traits>
#include <string>

DEBBY__NAMESPACE_BEGIN

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
     *
     * @note Numeration of columns starts from 1.
     */
    DEBBY__EXPORT std::string column_name (int column) const noexcept;

    /**
     * Steps to next record.
     */
    DEBBY__EXPORT void next ();

    /**
     * @return Column content or @c nullopt if column contains null value.
     *
     * @note Numeration of columns starts from 1.
     */
    template <typename T>
    DEBBY__EXPORT
    pfs::optional<std::enable_if_t<std::is_arithmetic<T>::value || std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>>
    get (int column, error * perr = nullptr);

    template <typename T>
    DEBBY__EXPORT
    pfs::optional<std::enable_if_t<std::is_arithmetic<T>::value || std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>>
    get (std::string const & column, error * perr = nullptr);

    template <typename T>
    pfs::optional<std::enable_if_t<!std::is_arithmetic<T>::value && !std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>>
    get (int const & column, error * perr = nullptr)
    {
        using affinity_type = typename value_type_affinity<std::decay_t<T>>::affinity_type;
        error err;
        auto affinity_value_opt = this->template get<affinity_type>(column, & err);

        if (err) {
            pfs::throw_or(perr, std::move(err));
            return T{};
        }

        if (!affinity_value_opt)
            return pfs::nullopt;

        return value_type_affinity<std::decay_t<T>>::cast(*affinity_value_opt, perr);
    }

    template <typename T>
    pfs::optional<std::enable_if_t<!std::is_arithmetic<T>::value && !std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>>
    get (std::string const & column, error * perr = nullptr)
    {
        using affinity_type = typename value_type_affinity<std::decay_t<T>>::affinity_type;
        error err;
        auto affinity_value_opt = this->template get<affinity_type>(column, & err);

        if (err) {
            pfs::throw_or(perr, std::move(err));
            return T{};
        }

        if (!affinity_value_opt)
            return pfs::nullopt;

        return value_type_affinity<std::decay_t<T>>::cast(*affinity_value_opt, perr);
    }

    /**
     * @return Column content or @a default_value if column contains null value.
     */
    template <typename T, typename ColumnType>
    T get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    {
        error err;
        auto valopt = get<T>(column, & err);

        if (!err) {
            if (!valopt)
                return default_value;

            return *valopt;
        }

        pfs::throw_or(perr, std::move(err));
        return default_value;
    }

    // FIXME REMOVE
    // template <typename T, typename ColumnType>
    // inline pfs::optional<typename std::enable_if<std::is_integral<T>::value, T>::type>
    // get (ColumnType const & column, error * perr = nullptr)
    // {
    //     auto opt = get_int64(column, perr);
    //     return opt ? pfs::make_optional(static_cast<T>(*opt)) : pfs::nullopt;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline pfs::optional<typename std::enable_if<std::is_floating_point<T>::value, T>::type>
    // get (ColumnType const & column, error * perr = nullptr)
    // {
    //     auto opt = get_double(column, perr);
    //     return opt ? pfs::make_optional(static_cast<T>(*opt)) : pfs::nullopt;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline pfs::optional<typename std::enable_if<std::is_same<std::decay_t<T>, std::string>::value, T>::type>
    // get (ColumnType const & column, error * perr = nullptr)
    // {
    //     auto opt = get_string(column, perr);
    //     return opt ? pfs::make_optional(*opt) : pfs::nullopt;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline pfs::optional<typename std::enable_if<std::is_enum<T>::value, T>::type>
    // get (ColumnType const & column, error * perr = nullptr)
    // {
    //     auto opt = get_int64(column, perr);
    //
    //     if (opt) {
    //         auto x = static_cast<typename std::underlying_type<T>::type>(*opt);
    //         return static_cast<T>(x);
    //     }
    //
    //     return pfs::nullopt;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline pfs::optional<typename std::enable_if<std::is_same<std::decay_t<T>, pfs::universal_id>::value, T>::type>
    // get (ColumnType const & column, error * perr = nullptr)
    // {
    //     auto opt = get_string(column, perr);
    //
    //     if (opt) {
    //         auto id = pfs::from_string<pfs::universal_id>(*opt);
    //
    //         if (id != pfs::universal_id{})
    //             return pfs::make_optional(id);
    //     }
    //
    //     return pfs::nullopt;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline pfs::optional<typename std::enable_if<std::is_same<std::decay_t<T>, pfs::utc_time>::value, T>::type>
    // get (ColumnType const & column, error * perr = nullptr)
    // {
    //     auto opt = get_int64(column, perr);
    //
    //     if (opt)
    //         return pfs::utc_time{std::chrono::milliseconds{*opt}};
    //
    //     return pfs::nullopt;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline pfs::optional<typename std::enable_if<std::is_same<std::decay_t<T>, pfs::local_time>::value, T>::type>
    // get (ColumnType const & column, error * perr = nullptr)
    // {
    //     auto opt = get_int64(column, perr);
    //
    //     if (opt)
    //         return pfs::local_time{std::chrono::milliseconds{*opt}};
    //
    //     return pfs::nullopt;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline typename std::enable_if<std::is_integral<T>::value, T>::type
    // get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    // {
    //     auto opt = get_int64(column, perr);
    //     return opt ? static_cast<T>(*opt) : default_value;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
    // get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    // {
    //     auto opt = get_double(column, perr);
    //     return opt ? static_cast<T>(*opt) : default_value;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline typename std::enable_if<std::is_same<std::decay_t<T>, std::string>::value, T>::type
    // get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    // {
    //     auto opt = get_string(column, perr);
    //     return opt ? *opt : default_value;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline typename std::enable_if<std::is_enum<T>::value, T>::type
    // get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    // {
    //     auto opt = get<T>(column, perr);
    //     return opt ? *opt : default_value;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline typename std::enable_if<std::is_same<std::decay_t<T>, pfs::universal_id>::value, T>::type
    // get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    // {
    //     auto opt = get<pfs::universal_id>(column, perr);
    //     return opt ? *opt : default_value;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline typename std::enable_if<std::is_same<std::decay_t<T>, pfs::utc_time>::value, T>::type
    // get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    // {
    //     auto opt = get<pfs::utc_time>(column, perr);
    //     return opt ? *opt : default_value;
    // }
    //
    // template <typename T, typename ColumnType>
    // inline typename std::enable_if<std::is_same<std::decay_t<T>, pfs::local_time>::value, T>::type
    // get_or (ColumnType const & column, T const & default_value, error * perr = nullptr)
    // {
    //     auto opt = get<pfs::local_time>(column, perr);
    //     return opt ? *opt : default_value;
    // }
};

DEBBY__NAMESPACE_END

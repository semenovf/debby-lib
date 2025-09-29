////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2024.11.04 V2 started.
//      2025.09.29 Changed set/get implementation.
//                 Added support for custom types.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "backend_enum.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "namespace.hpp"
#include "relational_database.hpp"
#include "pfs/string_view.hpp"
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

DEBBY__NAMESPACE_BEGIN

template <backend_enum Backend>
class keyvalue_database
{
public:
    class impl;

public:
    using string_view = pfs::string_view;
    using key_type = std::string;

private:
    std::unique_ptr<impl> _d;

public:
    DEBBY__EXPORT keyvalue_database ();
    DEBBY__EXPORT keyvalue_database (impl && d) noexcept;
    DEBBY__EXPORT keyvalue_database (keyvalue_database && other) noexcept;
    DEBBY__EXPORT ~keyvalue_database ();

    DEBBY__EXPORT keyvalue_database & operator = (keyvalue_database && other) noexcept;

    keyvalue_database (keyvalue_database const & other) = delete;
    keyvalue_database & operator = (keyvalue_database const & other) = delete;

public:
    /**
     * Checks if database is open.
     */
    inline operator bool () const noexcept
    {
        return _d != nullptr;
    }

    /**
     * Clear all records from @a table.
     */
    DEBBY__EXPORT void clear (error * perr = nullptr);

    /**
     * Removes entry associated with @a key from database.
     *
     * @throw debby::error()
     */
    DEBBY__EXPORT void remove (key_type const & key, error * perr = nullptr);

    /**
     * Stores character sequence @a value with length @a len associated
     * with @a key into database.
     */
    DEBBY__EXPORT void set (key_type const & key, char const * value, std::size_t len
        , error * perr = nullptr);

    /**
     * Stores arithmetic type @a value associated with @a key into database.
     */
    template <typename T>
    DEBBY__EXPORT
    std::enable_if_t<std::is_arithmetic<T>::value, void>
    set (key_type const & key, T value, error * perr = nullptr);

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, void>
    set (key_type const & key, T const & value, error * perr = nullptr)
    {
        set(key, keyvalue_affinity<std::decay_t<T>>::cast(value), perr);
    }

    /**
     * Stores string @a value associated with @a key into database.
     */
    void set (key_type const & key, std::string const & value, error * perr = nullptr)
    {
        set(key, value.data(), value.size(), perr);
    }

    /**
     * Stores string view @a value associated with @a key into database.
     */
    void set (key_type const & key, string_view value, error * perr = nullptr)
    {
        set(key, value.data(), value.size(), perr);
    }

    /**
     * Stores C-string @a value associated with @a key into database.
     */
    void set (key_type const & key, char const * value, error * perr = nullptr)
    {
        set(key, value, std::strlen(value), perr);
    }

    template <typename T>
    DEBBY__EXPORT
    std::enable_if_t<std::is_arithmetic<T>::value || std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>
    get (key_type const & key, error * perr = nullptr);

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value && !std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>
    get (key_type const & key, error * perr = nullptr)
    {
        using affinity_type = typename keyvalue_affinity<std::decay_t<T>>::affinity_type;
        error err;
        auto affinity_value = this->template get<affinity_type>(key, & err);

        if (err) {
            pfs::throw_or(perr, std::move(err));
            return T{};
        }

        return keyvalue_affinity<std::decay_t<T>>::cast(affinity_value, perr);
    }

    template <typename T>
    T get_or (key_type const & key, T const & default_value, error * perr = nullptr)
    {
        error err;
        auto result = get<T>(key, & err);

        if (!err)
            return result;

        if (make_error_code(errc::bad_value) == err.code())
            return default_value;

        if (make_error_code(errc::key_not_found) == err.code())
            return default_value;

        pfs::throw_or(perr, std::move(err));
        return default_value;
    }

public:
    template <typename ...Args>
    static keyvalue_database make (Args &&... args);

    /**
     * Wipes database (e.g. removes files associated with database if possible).
     *
     * @return @c true if removing was successful, @c false otherwise.
     */
    template <typename ...Args>
    static bool wipe (Args &&... args);
};

DEBBY__NAMESPACE_END

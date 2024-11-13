////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2024.11.04 V2 started.
////////////////////////////////////////////////////////////////////////////////
#pragma once
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

private:
    DEBBY__EXPORT void set_arithmetic (key_type const & key, std::int64_t value, std::size_t size, error * perr = nullptr);
    DEBBY__EXPORT void set_arithmetic (key_type const & key, double value, std::size_t size, error * perr = nullptr);
    DEBBY__EXPORT void set_chars (key_type const & key, char const * data, std::size_t size, error * perr = nullptr);

    DEBBY__EXPORT std::int64_t get_int64 (key_type const & key, error * perr = nullptr);
    DEBBY__EXPORT double get_double (key_type const & key, error * perr = nullptr);
    DEBBY__EXPORT std::string get_string (key_type const & key, error * perr = nullptr);

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
     * Stores arithmetic type @a value associated with @a key into database.
     */
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, void>::type
    set (key_type const & key, T value, error * perr = nullptr)
    {
        set_arithmetic(key, static_cast<std::int64_t>(value), sizeof(T), perr);
    }

    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value, void>::type
    set (key_type const & key, T value, error * perr = nullptr)
    {
        set_arithmetic(key, value, sizeof(T), perr);
    }

    /**
     * Stores string @a value associated with @a key into database.
     */
    void set (key_type const & key, std::string const & value, error * perr = nullptr)
    {
        set_chars(key, value.c_str(), value.size(), perr);
    }

    /**
     * Stores string view @a value associated with @a key into database.
     */
    void set (key_type const & key, string_view value, error * perr = nullptr)
    {
        set_chars(key, value.data(), value.size(), perr);
    }

    /**
     * Stores character sequence @a value with length @a len associated
     * with @a key into database.
     */
    void set (key_type const & key, char const * value, std::size_t len, error * perr = nullptr)
    {
        set_chars(key, value, len, perr);
    }

    /**
     * Stores C-string @a value associated with @a key into database.
     */
    void set (key_type const & key, char const * value, error * perr = nullptr)
    {
        set_chars(key, value, std::strlen(value), perr);
    }

    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    get (key_type const & key, error * perr = nullptr)
    {
        return static_cast<T>(get_int64(key, perr));
    }

    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value, T>::type
    get (key_type const & key, error * perr = nullptr)
    {
        return static_cast<T>(get_double(key, perr));
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, std::string>::value, T>::type
    get (key_type const & key, error * perr = nullptr)
    {
        return get_string(key, perr);
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

    /**
     * Removes entry associated with @a key from database.
     *
     * @throw debby::error()
     */
    DEBBY__EXPORT void remove (key_type const & key, error * perr = nullptr);

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

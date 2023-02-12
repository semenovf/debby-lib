////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include "exports.hpp"
#include "unified_value.hpp"
#include "pfs/fmt.hpp"
#include "pfs/string_view.hpp"
#include <string>
#include <cstring>
#include <memory>

namespace debby {

template <typename Backend>
class keyvalue_database
{
    using rep_type = typename Backend::rep_type;

public:
    using key_type   = typename Backend::key_type;
    using value_type = unified_value;

private:
    rep_type _rep;

private:
    DEBBY__EXPORT keyvalue_database (rep_type && rep);
    keyvalue_database (keyvalue_database const & other) = delete;
    keyvalue_database & operator = (keyvalue_database const & other) = delete;
    keyvalue_database & operator = (keyvalue_database && other) = delete;

    DEBBY__EXPORT void set_arithmetic (key_type const & key, std::intmax_t value
        , std::size_t size, error * perr = nullptr);

    DEBBY__EXPORT void set_arithmetic (key_type const & key, double value
        , std::size_t size, error * perr = nullptr);

    DEBBY__EXPORT void set_arithmetic (key_type const & key, float value
        , std::size_t size, error * perr = nullptr);

    DEBBY__EXPORT void set_chars (key_type const & key, char const * data
        , std::size_t size, error * perr = nullptr);

    DEBBY__EXPORT void set_blob (key_type const & key, char const * data
        , std::size_t size, error * perr = nullptr);

    DEBBY__EXPORT std::intmax_t get_integer (key_type const & key, error * perr = nullptr) const;
    DEBBY__EXPORT float get_float (key_type const & key, error * perr = nullptr) const;
    DEBBY__EXPORT double get_double (key_type const & key, error * perr = nullptr) const;
    DEBBY__EXPORT std::string get_string (key_type const & key, error * perr = nullptr) const;
    DEBBY__EXPORT blob_t get_blob (key_type const & key, error * perr = nullptr) const;

public:
    keyvalue_database () = default;
    DEBBY__EXPORT keyvalue_database (keyvalue_database && other);
    DEBBY__EXPORT ~keyvalue_database ();

public:
    /**
     * Checks if database is open.
     */
    DEBBY__EXPORT operator bool () const noexcept;

    /**
     * Stores arithmetic type @a value associated with @a key into database.
     */
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, void>::type
    set (key_type const & key, T value, error * perr = nullptr)
    {
        set_arithmetic(key, static_cast<std::intmax_t>(value), sizeof(T), perr);
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
    void set (key_type const & key, pfs::string_view value, error * perr = nullptr)
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

    /**
     * Stores blob @a value associated with @a key into database.
     */
    void set (key_type const & key, blob_t const & value, error * perr = nullptr)
    {
        set_blob(key, value, perr);
    }

    /**
     */
    template <typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    get (key_type const & key, error * perr = nullptr) const
    {
        return static_cast<T>(get_integer(key, perr));
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, float>::value, T>::type
    get (key_type const & key, error * perr = nullptr) const
    {
        return static_cast<T>(get_float(key, perr));
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, double>::value, T>::type
    get (key_type const & key, error * perr = nullptr) const
    {
        return static_cast<T>(get_double(key, perr));
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, std::string>::value, T>::type
    get (key_type const & key, error * perr = nullptr) const
    {
        return get_string(key, perr);
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, blob_t>::value, T>::type
    get (key_type const & key, error * perr = nullptr) const
    {
        return get_blob(key, perr);
    }

    template <typename T>
    T get_or (key_type const & key, T const & default_value, error * perr = nullptr) const
    {
        error err;
        auto result = get<T>(key, & err);

        if (!err)
            return result;

        if (make_error_code(errc::bad_value) == err.code())
            return default_value;

        if (make_error_code(errc::key_not_found) == err.code())
            return default_value;

        if (perr)
            *perr = err;
        else
            throw err;

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
    static keyvalue_database make (Args &&... args)
    {
        return keyvalue_database{Backend::make_kv(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<keyvalue_database> make_unique (Args &&... args)
    {
        auto ptr = new keyvalue_database {Backend::make_kv(std::forward<Args>(args)...)};
        return std::unique_ptr<keyvalue_database>(ptr);
    }
};

} // namespace debby

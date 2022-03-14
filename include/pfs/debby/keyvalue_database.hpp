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
#include "unified_value.hpp"
#include "pfs/fmt.hpp"
#include <string>
#include <cstring>
#include <memory>

namespace debby {

template <typename Backend>
class keyvalue_database final
{
    using rep_type = typename Backend::rep_type;

public:
    using key_type   = typename Backend::key_type;
    using value_type = unified_value;

private:
    rep_type _rep;

private:
    keyvalue_database () = delete;
    keyvalue_database (rep_type && rep);
    keyvalue_database (keyvalue_database const & other) = delete;
    keyvalue_database & operator = (keyvalue_database const & other) = delete;
    keyvalue_database & operator = (keyvalue_database && other) = delete;

public:
    keyvalue_database (keyvalue_database && other);
    ~keyvalue_database ();

private:
    result_status fetch (key_type const & key, value_type & value) const noexcept;

public:
    /**
     * Checks if database is open.
     */
    operator bool () const noexcept;

    /**
     * Drops database (delete all tables).
     *
     * @throw debby::error(errc::backend_error) on backend failure.
     */
    void clear ();

    /**
     * Stores arithmetic type @a value associated with @a key into database.
     */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    set (key_type const & key, T value)
    {
        rep_type::template set<T>(& _rep, key, value);
    }

    /**
     * Stores string @a value associated with @a key into database.
     *
     * @throw debby::error()
     */
    void set (key_type const & key, std::string const & value);

    /**
     * Stores character sequence @a value with length @a len associated
     * with @a key into database.
     */
    void set (key_type const & key, char const * value, std::size_t len);

    /**
     * Stores C-string @a value associated with @a key into database.
     */
    void set (key_type const & key, char const * value)
    {
        return set(key, value, std::strlen(value));
    }

    /**
     * Stores blob @a value associated with @a key into database.
     */
    void set (key_type const & key, blob_t const & value);

    /**
     */
    value_type fetch (key_type const & key) const
    {
        value_type value;
        auto res = fetch(key, value);

        if (!res.ok())
            DEBBY__THROW(res);

        return value;
    }

    /**
     */
    template <typename T>
    T get (key_type const & key)
    {
        value_type value;
        auto res = fetch(key, value);

        if (!res.ok())
            DEBBY__THROW(res);

        auto ptr = get_if<T>(& value);

        if (!ptr) {
            error err {make_error_code(errc::bad_value)
                , fmt::format("unsuitable data stored by key: {}", key)
            };

            DEBBY__THROW(err);
        }

        return std::move(*ptr);
    }

    template <typename T>
    T get_or (key_type const & key, T const & default_value)
    {
        value_type value;
        auto res = fetch(key, value);

        if (!res.ok()) {
            if (res.code().value() == static_cast<int>(errc::key_not_found))
                return default_value;

            DEBBY__THROW(res);
        }

        auto ptr = get_if<T>(& value);

        if (!ptr)
            return default_value;

        return std::move(*ptr);
    }

    /**
     * Removes entry associated with @a key from database.
     *
     * @throw debby::error()
     */
    void remove (key_type const & key);

public:
    template <typename ...Args>
    static keyvalue_database make (Args &&... args)
    {
        return keyvalue_database{Backend::make(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<keyvalue_database> make_unique (Args &&... args)
    {
        auto ptr = new keyvalue_database {Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<keyvalue_database>(ptr);
    }
};

} // namespace debby

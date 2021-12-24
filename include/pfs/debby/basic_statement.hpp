////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include <string>
#include <vector>

namespace debby {

template <typename Impl, typename Traits>
class basic_statement
{
public:
    using impl_type   = Impl;
    using result_type = typename Traits::result_type;

protected:
    basic_statement () = default;
    ~basic_statement () = default;
    basic_statement (basic_statement const &) = delete;
    basic_statement & operator = (basic_statement const &) = delete;
    basic_statement (basic_statement && other) = default;
    basic_statement & operator = (basic_statement && other) = default;

public:
    operator bool () const noexcept
    {
        return static_cast<Impl const*>(this)->is_prepared();
    }

    /**
     * Executes prepared statement
     */
    result_type exec (error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->exec_impl(perr);
    }

    int rows_affected () const
    {
        return static_cast<Impl const *>(this)->rows_affected_impl();
    }

    /**
     * Set the @a placeholder to be bound to arithmetic type (conformant the
     * std::is_arithmetic) @a value in the prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, bool>::type
    bind (std::string const & placeholder, T value, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder, value, perr);
    }

    /**
     * Set the @a placeholder to be bound to string @a value in the prepared
     * statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    bool bind (std::string const & placeholder, std::string const & value
        , bool static_value = false, error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder, value
            , static_value, perr);
    }

    /**
     * Set the @a placeholder to be bound to charcater sequence @a value with
     * length @a len in the prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    bool bind (std::string const & placeholder
        , char const * value, std::size_t len
        , bool static_value = false
        , error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder
            , value, len, static_value, perr);
    }

    /**
     * Set the @a placeholder to be bound to C-string @a value with in the
     * prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    bool bind (std::string const & placeholder, char const * value
        , bool static_value = false
        , error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder
            , value, std::strlen(value), static_value, perr);
    }

    /**
     * Set the @a placeholder to be bound to binary data @a value with in the
     * prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    bool bind (std::string const & placeholder
        , std::vector<std::uint8_t> const & value
        , bool static_value = false
        , error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder
            , value, static_value, perr);
    }

    /**
     * Set the @a placeholder to be bound to @c nullptr @a value with in the
     * prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    bool bind (std::string const & placeholder
        , std::nullptr_t value
        , error * perr = nullptr)
    {
        return static_cast<Impl*>(this)->bind_impl(placeholder, value, perr);
    }
};

} // namespace debby

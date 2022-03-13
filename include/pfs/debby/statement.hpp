////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "error.hpp"
#include <string>
#include <vector>

namespace debby {

template <typename Backend>
class statement final
{
    using rep_type = typename Backend::rep_type;

public:
    using result_type = typename Backend::result_type;

private:
    rep_type _rep;

private:
    statement () = delete;
    statement (rep_type && rep);
    statement (statement const & other) = delete;
    statement & operator = (statement const & other) = delete;
    statement & operator = (statement && other) = delete;

public:
    statement (statement && other);
    ~statement ();

public:
    operator bool () const noexcept;

    /**
     * Executes prepared statement
     */
    result_type exec ();

    /**
     */
    int rows_affected () const;

    /**
     * Set the @a placeholder to be bound to @c nullptr @a value with in the
     * prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    void bind (std::string const & placeholder, std::nullptr_t);

    /**
     * Set the @a placeholder to be bound to arithmetic type (conformant the
     * std::is_arithmetic) @a value in the prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    bind (std::string const & placeholder, T value);

    /**
     * Set the @a placeholder to be bound to character sequence @a value with
     * length @a len in the prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    void bind (std::string const & placeholder
        , char const * value
        , std::size_t len
        , bool transient = true);

    /**
     * Set the @a placeholder to be bound to C-string @a value with in the
     * prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    void bind (std::string const & placeholder, char const * value, bool transient = true)
    {
        bind(placeholder, value, std::strlen(value), transient);
    }

    /**
     * Set the @a placeholder to be bound to string @a value in the prepared
     * statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    void bind (std::string const & placeholder
        , std::string const & value
        , bool transient = true)
    {
        bind(placeholder, value.c_str(), value.size(), transient);
    }

    /**
     * Set the @a placeholder to be bound to binary data @a value with in the
     * prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    void bind (std::string const & placeholder
        , std::vector<std::uint8_t> const & value
        , bool transient = true);

public:
    template <typename ...Args>
    static statement make (Args &&... args)
    {
        return statement{Backend::make(std::forward<Args>(args)...)};
    }

    template <typename ...Args>
    static std::unique_ptr<statement> make_unique (Args &&... args)
    {
        auto ptr = new statement {Backend::make(std::forward<Args>(args)...)};
        return std::unique_ptr<statement>(ptr);
    }
};

} // namespace debby

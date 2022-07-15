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
#include "exports.hpp"
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
    DEBBY__EXPORT statement (rep_type && rep);
    statement (statement const & other) = delete;
    statement & operator = (statement const & other) = delete;
    statement & operator = (statement && other) = delete;

public:
    DEBBY__EXPORT statement (statement && other) noexcept;
    DEBBY__EXPORT ~statement ();

public:
    DEBBY__EXPORT operator bool () const noexcept;

    /**
     * Executes prepared statement
     */
    DEBBY__EXPORT result_type exec ();

    /**
     */
    DEBBY__EXPORT int rows_affected () const;

    /**
     */
    template <typename T>
    void bind (std::string const & placeholder, T && value)
    {
        Backend::template bind<T>(& _rep, placeholder, std::forward<T>(value));
    }

    /**
     * Set the @a placeholder to be bound to blob @a value with length @a len
     * in the prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    DEBBY__EXPORT void bind (std::string const & placeholder
        , char const * blob
        , std::size_t len
        , bool transient);

    /**
     * Set the @a placeholder to be bound to string @a value in the prepared
     * statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    DEBBY__EXPORT void bind (std::string const & placeholder
        , std::string const & value
        , bool transient);

    /**
     * Set the @a placeholder to be bound to C-string @a value with in the
     * prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    DEBBY__EXPORT void bind (std::string const & placeholder
        , char const * value
        , bool transient);

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

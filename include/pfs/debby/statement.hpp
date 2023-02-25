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
#include "pfs/string_view.hpp"
#include <string>
#include <vector>

namespace debby {

using pfs::string_view;

enum class transient_enum { no, yes };

template <typename Backend>
class statement
{
    using rep_type = typename Backend::rep_type;

public:
    using result_type = typename Backend::result_type;

private:
    rep_type _rep;

private:
    DEBBY__EXPORT statement (rep_type && rep);
    statement (statement const & other) = delete;
    statement & operator = (statement const & other) = delete;
    statement & operator = (statement && other) = delete;

public:
    DEBBY__EXPORT statement () = delete;
    DEBBY__EXPORT statement (statement && other) noexcept;
    DEBBY__EXPORT ~statement ();

public:
    DEBBY__EXPORT operator bool () const noexcept;

    /**
     * Executes prepared statement
     */
    DEBBY__EXPORT result_type exec (error * perr = nullptr);

    /**
     */
    DEBBY__EXPORT int rows_affected () const;

    /**
     */
    template <typename T>
    bool bind (int index, T && value, error * perr = nullptr)
    {
        return Backend::template bind<T>(& _rep, index, std::forward<T>(value), perr);
    }

    /**
     */
    template <typename T>
    bool bind (std::string const & placeholder, T && value, error * perr = nullptr)
    {
        return Backend::template bind<T>(& _rep, placeholder, std::forward<T>(value), perr);
    }

    /**
     */
    DEBBY__EXPORT bool bind (int index, std::string const & value
        , transient_enum transient, error * perr = nullptr);

    /**
     * Set the @a placeholder to be bound to string @a value in the prepared
     * statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    DEBBY__EXPORT bool bind (std::string const & placeholder
        , std::string const & value, transient_enum transient
        , error * perr = nullptr);

    /**
     */
    DEBBY__EXPORT bool bind (int index, string_view value
        , transient_enum transient, error * perr = nullptr);

    /**
     * Set the @a placeholder to be bound to string @a value in the prepared
     * statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    DEBBY__EXPORT bool bind (std::string const & placeholder, string_view value
        , transient_enum transient, error * perr = nullptr);

    /**
     */
    DEBBY__EXPORT bool bind (int index, char const * value
        , transient_enum transient, error * perr = nullptr);

    /**
     * Set the @a placeholder to be bound to C-string @a value with in the
     * prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    DEBBY__EXPORT bool bind (std::string const & placeholder
        , char const * value, transient_enum transient, error * perr = nullptr);

    /**
     * Set the @a index to be bound to @a blob in the prepared statement.
     */
    DEBBY__EXPORT bool bind (int index, std::uint8_t const * blob, std::size_t len
        , transient_enum transient, error * perr = nullptr);

    /**
     * Set the @a index to be bound to @a blob in the prepared statement.
     */
    inline bool bind (int index, char const * blob, std::size_t len
        , transient_enum transient, error * perr = nullptr)
    {
        return bind(index, reinterpret_cast<std::uint8_t const *>(blob), len
            , transient, perr);
    }

    /**
     * Set the @a index to be bound to @a blob in the prepared statement.
     */
    inline bool bind (int index, std::vector<std::uint8_t> blob
        , transient_enum transient, error * perr = nullptr)
    {
        return bind(index, blob.data(), blob.size(), transient, perr);
    }

    /**
     * Set the @a index to be bound to @a blob in the prepared statement.
     */
    inline bool bind (int index, std::vector<char> const & blob
        , transient_enum transient, error * perr = nullptr)
    {
        return bind(index, reinterpret_cast<std::uint8_t const *>(blob.data())
            , blob.size(), transient, perr);
    }

    /**
     * Set the @a placeholder to be bound to @a blob with length @a len
     * in the prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     */
    DEBBY__EXPORT bool bind (std::string const & placeholder
        , std::uint8_t const * blob, std::size_t len
        , transient_enum transient, error * perr = nullptr);

    inline bool bind (std::string const & placeholder
        , char const * blob, std::size_t len
        , transient_enum transient, error * perr = nullptr)
    {
        return bind(placeholder, reinterpret_cast<std::uint8_t const *>(blob)
            , len, transient, perr);
    }

    inline bool bind (std::string const & placeholder
        , std::vector<std::uint8_t> const & blob, transient_enum transient
        , error * perr = nullptr)
    {
        return bind(placeholder, blob.data(), blob.size(), transient, perr);
    }

    inline bool bind (std::string const & placeholder
        , std::vector<char> const & blob, transient_enum transient
        , error * perr = nullptr)
    {
        return bind(placeholder, reinterpret_cast<std::uint8_t const *>(blob.data())
            , blob.size(), transient, perr);
    }

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

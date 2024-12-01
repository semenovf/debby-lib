////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2024.10.29 V2 started.
//      2024.10.30 Fixed API.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "backend_enum.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "namespace.hpp"
#include "result.hpp"
#include <pfs/time_point.hpp>
#include <pfs/universal_id.hpp>
#include <cstdint>
#include <string>
#include <type_traits>

DEBBY__NAMESPACE_BEGIN

template <backend_enum Backend>
class statement
{
public:
    class impl;
    using result_type = result<Backend>;

private:
    impl * _d {nullptr};

public:
    DEBBY__EXPORT statement ();
    DEBBY__EXPORT statement (impl && d);
    DEBBY__EXPORT statement (statement && other) noexcept;
    DEBBY__EXPORT ~statement ();

    statement (statement const & other) = delete;
    statement & operator = (statement const & other) = delete;
    statement & operator = (statement && other) = delete;

private:
    DEBBY__EXPORT bool bind_helper (int index, std::int64_t value, error * perr = nullptr);
    DEBBY__EXPORT bool bind_helper (int index, double value, error * perr = nullptr);
    DEBBY__EXPORT bool bind_helper (char const * placeholder, std::int64_t value, error * perr = nullptr);
    DEBBY__EXPORT bool bind_helper (char const * placeholder, double value, error * perr = nullptr);

public:
    inline operator bool () const noexcept
    {
        return _d != nullptr;
    }

    /**
     * Resets prepared statement to its initial state, ready to be re-executed.
     */
    DEBBY__EXPORT void reset (error * perr = nullptr);

    /**
     * Executes prepared statement
     */
    DEBBY__EXPORT result_type exec (error * perr = nullptr);

    template <typename T>
    inline typename std::enable_if<std::is_integral<T>::value, bool>::type
    bind (int index, T value, error * perr = nullptr)
    {
        return bind_helper(index, static_cast<std::int64_t>(value), perr);
    }

    template <typename T>
    inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    bind (int index, T value, error * perr = nullptr)
    {
        return bind_helper(index, static_cast<double>(value), perr);
    }

    template <typename T>
    inline typename std::enable_if<std::is_enum<T>::value, bool>::type
    bind (int index, T value, error * perr = nullptr)
    {
        return bind_helper(index, static_cast<std::int64_t>(value), perr);
    }

    /**
     * @note Not all databases support placeholder. In this case an exception (@c errc::unsupported)
     * is thrown or an error is returned in @a *perr.
     */
    template <typename T>
    inline typename std::enable_if<std::is_integral<T>::value, bool>::type
    bind (char const * placeholder, T value, error * perr = nullptr)
    {
        return bind_helper(placeholder, static_cast<std::int64_t>(value), perr);
    }

    /**
     * @note Not all databases support placeholder. In this case an exception (@c errc::unsupported)
     * is thrown or an error is returned in @a *perr.
     */
    template <typename T>
    inline typename std::enable_if<std::is_floating_point<T>::value, bool>::type
    bind (char const * placeholder, T value, error * perr = nullptr)
    {
        return bind_helper(placeholder, static_cast<double>(value), perr);
    }

    template <typename T>
    inline typename std::enable_if<std::is_enum<T>::value, bool>::type
    bind (char const * placeholder, T value, error * perr = nullptr)
    {
        return bind_helper(placeholder, static_cast<std::int64_t>(value), perr);
    }

    DEBBY__EXPORT bool bind (int index, std::nullptr_t, error * perr = nullptr);
    DEBBY__EXPORT bool bind (char const * placeholder, std::nullptr_t, error * perr = nullptr);
    DEBBY__EXPORT bool bind (int index, std::string && value, error * perr = nullptr);

    /**
     * Set the @a placeholder to be bound to string @a value in the prepared statement.
     *
     * @details Placeholder mark (e.g :) must be included when specifying the
     *          placeholder name.
     *
     * @note Not all databases support placeholder. In this case an exception (@c errc::unsupported)
     * is thrown or an error is returned in @a *perr.
     */
    DEBBY__EXPORT bool bind (char const * placeholder, std::string && value, error * perr = nullptr);

    /**
     * Bind binary value (blob) to @a index. @a ptr must remain valid until either the
     * prepared statement is finalized or the same SQL parameter is bound to something else,
     * whichever occurs sooner.
     */
    DEBBY__EXPORT bool bind (int index, char const * ptr, std::size_t len, error * perr = nullptr);

    /**
     * Bind binary value (blob) to @a placholder. @a ptr must remain valid until either the
     * prepared statement is finalized or the same SQL parameter is bound to something else,
     * whichever occurs sooner.
     */
    DEBBY__EXPORT bool bind (char const * placeholder, char const * ptr, std::size_t len, error * perr = nullptr);

    DEBBY__EXPORT bool bind (int index, pfs::universal_id uuid, error * perr = nullptr);
    DEBBY__EXPORT bool bind (char const * placeholder, pfs::universal_id uuid, error * perr = nullptr);
    DEBBY__EXPORT bool bind (int index, pfs::utc_time time, error * perr = nullptr);
    DEBBY__EXPORT bool bind (char const * placeholder, pfs::utc_time time, error * perr = nullptr);
    DEBBY__EXPORT bool bind (int index, pfs::local_time time, error * perr = nullptr);
    DEBBY__EXPORT bool bind (char const * placeholder, pfs::local_time time, error * perr = nullptr);

};

DEBBY__NAMESPACE_END

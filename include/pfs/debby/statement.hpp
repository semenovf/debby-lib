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
#include "namespace.hpp"
#include "affinity_traits.hpp"
#include "backend_enum.hpp"
#include "error.hpp"
#include "exports.hpp"
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
    DEBBY__EXPORT statement & operator = (statement && other);

    statement (statement const & other) = delete;

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

    /**
     * @note Numeration of columns starts from 1.
     */
    template <typename T>
    DEBBY__EXPORT
    std::enable_if_t<std::is_arithmetic<T>::value
        || std::is_same<std::decay_t<T>, std::string>::value, bool>
    bind (int index, T const & value, error * perr = nullptr);

    /**
     * @note Not all databases support placeholder. In this case an exception (@c errc::unsupported)
     * is thrown or an error is returned in @a *perr.
     */
    template <typename T>
    DEBBY__EXPORT
    std::enable_if_t<std::is_arithmetic<T>::value
        || std::is_same<std::decay_t<T>, std::string>::value, bool>
    bind (char const * placeholder, T const & value, error * perr = nullptr);

    /**
     * Bind custom type value to @a index.
     */
    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value
        && !std::is_same<std::decay_t<T>, std::string>::value, bool>
    bind (int index, T const & value, error * perr = nullptr)
    {
        using affinity_type = typename value_type_affinity<std::decay_t<T>>::affinity_type;
        return bind<affinity_type>(index, value_type_affinity<std::decay_t<T>>::cast(value), perr);
    }

    /**
     * Bind custom type value to @a placeholder.
     */
    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value
        && !std::is_same<std::decay_t<T>, std::string>::value, bool>
    bind (char const * placeholder, T const & value, error * perr = nullptr)
    {
        using affinity_type = typename value_type_affinity<std::decay_t<T>>::affinity_type;
        return bind<affinity_type>(placeholder, value_type_affinity<std::decay_t<T>>::cast(value), perr);
    }

    DEBBY__EXPORT bool bind (int index, std::nullptr_t, error * perr = nullptr);
    DEBBY__EXPORT bool bind (char const * placeholder, std::nullptr_t, error * perr = nullptr);

    /**
     * Bind binary value (blob) to @a index. @a ptr must remain valid until either the
     * prepared statement is finalized or the same SQL parameter is bound to something else,
     * whichever occurs sooner.
     */
    DEBBY__EXPORT bool bind (int index, char const * ptr, std::size_t len, error * perr = nullptr);

    DEBBY__EXPORT bool bind (char const * placeholder, char const * ptr, std::size_t len, error * perr = nullptr);

    /**
     * Bind string value to @a placholder. @a ptr must remain valid until either the
     * prepared statement is finalized or the same SQL parameter is bound to something else,
     * whichever occurs sooner.
     */
    DEBBY__EXPORT bool bind (int index, char const * ptr, error * perr = nullptr);

    DEBBY__EXPORT bool bind (char const * placeholder, char const * ptr, error * perr = nullptr);
};

DEBBY__NAMESPACE_END

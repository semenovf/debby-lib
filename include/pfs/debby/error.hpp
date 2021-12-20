////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/filesystem.hpp"
#include <functional>
#include <string>
#include <system_error>
#include <cstdio>

namespace debby {

namespace {
inline void assert_fail (char const * file, int line, char const * message)
{
    std::fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, message);
    std::terminate();
}
} // namespace

////////////////////////////////////////////////////////////////////////////////
// Error codes, category, exception class
////////////////////////////////////////////////////////////////////////////////
using error_code = std::error_code;

#ifndef DEBBY__ASSERT
#   ifdef NDEBUG
#       define DEBBY__ASSERT(condition, message)
#   else
#       define DEBBY__ASSERT(condition, message)                 \
            ((condition)                                        \
                ? (void)0                                       \
                : ::debby::assert_fail(__FILE__, __LINE__, (message)))
#   endif
#endif

#ifndef DEBBY__THROW
#   if DEBBY__EXCEPTIONS_ENABLED
#       define DEBBY__THROW(x) throw x
#   else
#       define DEBBY__THROW(x)                                                \
            do {                                                              \
                ::debby::assert_fail(__FILE__, __LINE__, (x).what().c_str()); \
            } while (false)
#   endif
#endif

// #if DEBBY__EXCEPTIONS_ENABLED
// #   define DEBBY_TRY try
// #   define DEBBY_CATCH(x) catch (x)
// #else
// #   define DEBBY_TRY if (true)
// #   define DEBBY_CATCH(x) if (false)
// #endif

enum class errc
{
      success = 0
    , bad_alloc
    , backend_error  // Error from underlying subsystem
                     // (i.e. sqlite3, RrocksDb,... specific errors)
    , database_not_found
    , bad_value      // Bad/unsuitable value stored
    , sql_error
    , invalid_argument
};

class error_category : public std::error_category
{
public:
    virtual char const * name () const noexcept override;
    virtual std::string message (int ev) const override;
};

inline std::error_category const & get_error_category ()
{
    static error_category instance;
    return instance;
}

inline std::error_code make_error_code (errc e)
{
    return std::error_code(static_cast<int>(e), get_error_category());
}

/**
 * @note This exception class may throw std::bad_alloc as it uses std::string.
 */
class error
{
private:
    std::error_code _ec;
    // FIXME Need separately-allocated reference-counted string representation
    // for this members.
    std::string _description;
    std::string _cause;

public:
    error () = default;

    error (std::error_code ec)
        : _ec(ec)
    {}

    error (std::error_code ec
        , std::string const & description
        , std::string const & cause)
        : _ec(ec)
        , _description(description)
        , _cause(cause)
    {}

    error (std::error_code ec
        , std::string const & description)
        : _ec(ec)
        , _description(description)
    {}

    // FIXME Need separately-allocated reference-counted string representation
    // for internal members.
    // See https://en.cppreference.com/w/cpp/error/runtime_error
    // "Because copying std::runtime_error is not permitted to throw exceptions,
    // this message is typically stored internally as a separately-allocated
    // reference-counted string. This is also why there is no constructor
    // taking std::string&&: it would have to copy the content anyway."

    error (error const & other ) /*noexcept*/ = default;
    error (error && other ) /*noexcept*/ = default;
    error & operator = (error const & other ) /*noexcept*/ = default;
    error & operator = (error && other ) /*noexcept*/ = default;

    operator bool () const noexcept
    {
        return !!_ec;
    }

    std::error_code code () const noexcept
    {
        return _ec;
    }

    std::string error_message () const noexcept
    {
        return _ec.message();
    }

    std::string const & description () const noexcept
    {
        return _description;
    }

    std::string const & cause () const noexcept
    {
        return _cause;
    }

    std::string what () const noexcept;
};

} // namespace debby

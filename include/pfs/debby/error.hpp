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

namespace pfs {
namespace debby {

////////////////////////////////////////////////////////////////////////////////
// Error codes, category, exception class
////////////////////////////////////////////////////////////////////////////////
using error_code = std::error_code;

enum class errc
{
      success = 0
    , backend_error  // Error from underlying subsystem
                     // (i.e. sqlite3, RrocksDb,... specific errors)
    , bad_alloc
    , database_already_open
    , database_not_found
    , bad_value      // Bad/unsuitable value stored
    , sql_error
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

class exception
{
public:
    static
#if PFS_DEBBY__EXCEPTIONS_ENABLED
    const
#endif
    std::function<void(exception &&)> failure;

private:
    std::error_code _ec;
    std::string _description;
    std::string _cause;

public:
    exception (std::error_code ec)
        : _ec(ec)
    {}

    exception (std::error_code ec
        , std::string const & description
        , std::string const & cause)
        : _ec(ec)
        , _description(description)
        , _cause(cause)
    {}

    exception (std::error_code ec
        , filesystem::path const & path)
        : _ec(ec)
        , _description(path.c_str())
    {}

    exception (std::error_code ec
        , filesystem::path const & path
        , std::string const & cause)
        : _ec(ec)
        , _description(path.c_str())
        , _cause(cause)
    {}


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

}} // namespace pfs::debby

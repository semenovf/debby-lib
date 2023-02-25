////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "exports.hpp"
#include "pfs/error.hpp"
#include "pfs/filesystem.hpp"
#include <functional>
#include <string>
#include <system_error>
#include <cstdio>

namespace debby {

enum class errc
{
      success = 0
    , bad_alloc
    , backend_error  // Error from underlying subsystem
                     // (i.e. sqlite3, RrocksDb,... specific errors)
    , database_not_found
    , key_not_found
    , column_not_found = key_not_found
    , bad_value      // Bad/unsuitable value stored
    , sql_error
    , invalid_argument
};

class error_category : public std::error_category
{
public:
    virtual DEBBY__EXPORT char const * name () const noexcept override;
    virtual DEBBY__EXPORT std::string message (int ev) const override;
};

DEBBY__EXPORT std::error_category const & get_error_category ();

inline std::error_code make_error_code (errc e)
{
    return std::error_code(static_cast<int>(e), get_error_category());
}

class error: public pfs::error
{
public:
#if _MSC_VER
    // MSVC 2022 C2512
    error () : pfs::error() {}
#else
    using pfs::error::error;
#endif

    error (errc ec)
        : pfs::error(make_error_code(ec))
    {}

    error (errc ec
        , std::string const & description
        , std::string const & cause)
        : pfs::error(make_error_code(ec), description, cause)
    {}

    error (errc ec
        , std::string const & description)
        : pfs::error(make_error_code(ec), description)
    {}
};

} // namespace debby

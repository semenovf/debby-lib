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
#include "sqlite3.h"
#include "utils.hpp"
#include "pfs/assert.hpp"
#include "pfs/debby/statement.hpp"
#include "pfs/debby/backend/sqlite3/result.hpp"
#include "pfs/debby/backend/sqlite3/statement.hpp"
#include <climits>

namespace debby {

static char const * NULL_HANDLER = "uninitialized statement handler";

namespace backend {
namespace sqlite3 {

statement::rep_type statement::make (native_type sth, bool cached)
{
    rep_type rep;
    rep.sth = sth;
    rep.cached = cached;
    return rep;
}

static bool bind_helper_func (statement::rep_type * rep, int index
    , std::function<int (int /*index*/)> && sqlite3_binder_func
    , error * perr)
{
    PFS__ASSERT(rep->sth, NULL_HANDLER);

    // In sqlite index must be started from 1
    int rc = sqlite3_binder_func(index + 1);

    if (SQLITE_OK != rc) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , build_errstr(rc, rep->sth)
            , current_sql(rep->sth)
        });

        return false;
    }

    return true;
}

static bool bind_helper_func (statement::rep_type * rep
    , std::string const & placeholder
    , std::function<int (int /*index*/)> && sqlite3_binder_func
    , error * perr)
{
    PFS__ASSERT(rep->sth, NULL_HANDLER);

    int index = sqlite3_bind_parameter_index(rep->sth, placeholder.c_str());

    if (index == 0) {
        pfs::throw_or(perr, error {
              errc::invalid_argument
            , std::string{"bad bind parameter name"}
            , placeholder
        });

        return false;
    }

    int rc = sqlite3_binder_func(index);

    if (SQLITE_OK != rc) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , build_errstr(rc, rep->sth)
            , current_sql(rep->sth)
        });

        return false;
    }

    return true;
}

template <>
bool
statement::bind_helper (statement::rep_type * rep
    , std::string const & placeholder
    , bool && value, error * perr)
{
    return bind_helper_func(rep, placeholder, [rep, value] (int index) {
        return sqlite3_bind_int(rep->sth, index, (value ? 1 : 0));
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, bool && value
    , error * perr)
{
    return bind_helper_func(rep, index, [rep, value] (int index) {
        return sqlite3_bind_int(rep->sth, index, (value ? 1 : 0));
    }, perr);
}

#define BIND_INDEX_INT_DEF(T)                                                  \
    template <>                                                                \
    bool                                                                       \
    statement::bind_helper (statement::rep_type * rep                          \
        , int index, T && value, error * perr)                                 \
    {                                                                          \
        return bind_helper_func(rep, index, [rep, value] (int index) {         \
            return sqlite3_bind_int(rep->sth, index                            \
                , static_cast<int>(value));                                    \
        }, perr);                                                              \
    }

#define BIND_INDEX_INT64_DEF(T)                                                \
    template <>                                                                \
    bool                                                                       \
    statement::bind_helper (statement::rep_type * rep                          \
        , int index, T && value, error * perr)                                 \
    {                                                                          \
        return bind_helper_func(rep, index, [rep, value] (int index) {         \
            return sqlite3_bind_int64(rep->sth                                 \
                , index                                                        \
                , static_cast<sqlite3_int64>(value));                          \
        }, perr);                                                              \
    }

#define BIND_PLACEHOLDER_INT_DEF(T)                                            \
    template <>                                                                \
    bool                                                                       \
    statement::bind_helper (statement::rep_type * rep                          \
        , std::string const & placeholder, T && value, error * perr)           \
    {                                                                          \
        return bind_helper_func(rep, placeholder, [rep, value] (int index) {   \
            return sqlite3_bind_int(rep->sth, index                            \
            , static_cast<int>(value));                                        \
        }, perr);                                                              \
    }

#define BIND_PLACEHOLDER_INT64_DEF(T)                                          \
    template <>                                                                \
    bool                                                                       \
    statement::bind_helper (statement::rep_type * rep                          \
        , std::string const & placeholder, T && value, error * perr)           \
    {                                                                          \
        return bind_helper_func(rep, placeholder, [rep, value] (int index) {   \
            return sqlite3_bind_int64(rep->sth                                 \
                , index                                                        \
                , static_cast<sqlite3_int64>(value));                          \
        }, perr);                                                              \
    }

BIND_INDEX_INT_DEF(char)
BIND_INDEX_INT_DEF(signed char)
BIND_INDEX_INT_DEF(unsigned char)
BIND_INDEX_INT_DEF(short)
BIND_INDEX_INT_DEF(unsigned short)
BIND_INDEX_INT_DEF(int)
BIND_INDEX_INT_DEF(unsigned int)

BIND_PLACEHOLDER_INT_DEF(char)
BIND_PLACEHOLDER_INT_DEF(signed char)
BIND_PLACEHOLDER_INT_DEF(unsigned char)
BIND_PLACEHOLDER_INT_DEF(short)
BIND_PLACEHOLDER_INT_DEF(unsigned short)
BIND_PLACEHOLDER_INT_DEF(int)
BIND_PLACEHOLDER_INT_DEF(unsigned int)

#if (defined(LONG_MAX) && LONG_MAX == 2147483647L)  \
        || (defined(_LONG_MAX__) && __LONG_MAX__ == 2147483647L)
    BIND_INDEX_INT_DEF(long)
    BIND_INDEX_INT_DEF(unsigned long)
    BIND_PLACEHOLDER_INT_DEF(long)
    BIND_PLACEHOLDER_INT_DEF(unsigned long)
#else
    BIND_INDEX_INT64_DEF(long)
    BIND_INDEX_INT64_DEF(unsigned long)
    BIND_PLACEHOLDER_INT64_DEF(long)
    BIND_PLACEHOLDER_INT64_DEF(unsigned long)
#endif

#if defined(LONG_LONG_MAX) || defined(__LONG_LONG_MAX__)
    BIND_INDEX_INT64_DEF(long long)
    BIND_INDEX_INT64_DEF(unsigned long long)
    BIND_PLACEHOLDER_INT64_DEF(long long)
    BIND_PLACEHOLDER_INT64_DEF(unsigned long long)
#endif

#if defined(_MSC_VER)
    BIND_INDEX_INT64_DEF(__int64)
    BIND_INDEX_INT64_DEF(unsigned __int64)
    BIND_PLACEHOLDER_INT64_DEF(__int64)
    BIND_PLACEHOLDER_INT64_DEF(unsigned __int64)
#endif

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, float && value
    , error * perr)
{
    return bind_helper_func(rep, index, [rep, value] (int index) {
        return sqlite3_bind_double(rep->sth, index, static_cast<double>(value));
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep
    , std::string const & placeholder, float && value, error * perr)
{
    return bind_helper_func(rep, placeholder, [rep, value] (int index) {
        return sqlite3_bind_double(rep->sth, index, static_cast<double>(value));
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, double && value
    , error * perr)
{
    return bind_helper_func(rep, index, [rep, value] (int index) {
        return sqlite3_bind_double(rep->sth, index, value);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep
    , std::string const & placeholder, double && value, error * perr)
{
    return bind_helper_func(rep, placeholder, [rep, value] (int index) {
        return sqlite3_bind_double(rep->sth, index, value);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index
    , std::nullptr_t &&, error * perr)
{
    return bind_helper_func(rep, index, [rep] (int index) {
        return sqlite3_bind_null(rep->sth, index);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep
    , std::string const & placeholder, std::nullptr_t &&, error * perr)
{
    return bind_helper_func(rep, placeholder, [rep] (int index) {
        return sqlite3_bind_null(rep->sth, index);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, std::string && value
    , error * perr)
{
    auto str = value.c_str();
    auto len = value.size();

    return backend::sqlite3::bind_helper_func(rep, index, [rep, str, len] (int index) {
        return sqlite3_bind_text(rep->sth, index, str
            , static_cast<int>(len)
            , SQLITE_TRANSIENT);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep
    , std::string const & placeholder, std::string && value, error * perr)
{
    auto str = value.c_str();
    auto len = value.size();

    return backend::sqlite3::bind_helper_func(rep, placeholder, [rep, str, len] (int index) {
        return sqlite3_bind_text(rep->sth, index, str
            , static_cast<int>(len)
            , SQLITE_TRANSIENT);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index
    , char const * && value, error * perr)
{
    return backend::sqlite3::bind_helper_func(rep, index, [rep, value] (int index) {
        return sqlite3_bind_text(rep->sth, index, value
            , static_cast<int>(std::strlen(value))
            , SQLITE_TRANSIENT);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep
    , std::string const & placeholder, char const * && value, error * perr)
{
    return backend::sqlite3::bind_helper_func(rep, placeholder, [rep, value] (int index) {
        return sqlite3_bind_text(rep->sth, index, value
            , static_cast<int>(std::strlen(value))
            , SQLITE_TRANSIENT);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, std::vector<char> && value
    , error * perr)
{
    auto data = value.data();
    auto len = value.size();

    return backend::sqlite3::bind_helper_func(rep, index, [rep, data, len] (int index) {
        return sqlite3_bind_blob(rep->sth, index, data, static_cast<int>(len), SQLITE_TRANSIENT);
    }, perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep
    , std::string const & placeholder, std::vector<char> && value, error * perr)
{
    auto data = value.data();
    auto len = value.size();

    return backend::sqlite3::bind_helper_func(rep, placeholder, [rep, data, len] (int index) {
        return sqlite3_bind_blob(rep->sth, index, data, static_cast<int>(len), SQLITE_TRANSIENT);
    }, perr);
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::statement

template <>
statement<BACKEND>::statement ()
{
    _rep = BACKEND::make(nullptr, false);
}

template <>
statement<BACKEND>::statement (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
statement<BACKEND>::statement (statement && other) noexcept
{
    _rep = std::move(other._rep);
    other._rep.sth = nullptr;
}

template <>
statement<BACKEND>::~statement ()
{
    if (_rep.sth) {
        if (!_rep.cached)
            sqlite3_finalize(_rep.sth);
        else
            sqlite3_reset(_rep.sth);
    }

    _rep.sth = nullptr;
}

template <>
statement<BACKEND>::operator bool () const noexcept
{
    return _rep.sth != nullptr;
}

template <>
int
statement<BACKEND>::rows_affected () const
{
    PFS__ASSERT(_rep.sth, NULL_HANDLER);

    auto dbh = sqlite3_db_handle(_rep.sth);

    PFS__ASSERT(dbh, NULL_HANDLER);

    //return sqlite3_changes64(_sth);
    return sqlite3_changes(dbh);
}

template <>
statement<BACKEND>::result_type
statement<BACKEND>::exec (error * perr)
{
    PFS__ASSERT(_rep.sth, NULL_HANDLER);

    std::error_code ec;
    backend::sqlite3::result::status status {backend::sqlite3::result::INITIAL};
    int rc = sqlite3_step(_rep.sth);

    switch (rc) {
        case SQLITE_ROW:
            status = backend::sqlite3::result::ROW;
            break;

        case SQLITE_DONE:
            status = backend::sqlite3::result::DONE;
            break;

        case SQLITE_CONSTRAINT:
        case SQLITE_ERROR: {
            status = backend::sqlite3::result::FAILURE;

            error err {
                  errc::sql_error
                , backend::sqlite3::build_errstr(rc, _rep.sth)
                , backend::sqlite3::current_sql(_rep.sth)
            };

            if (perr) {
                *perr = std::move(err);
                result_type::make(nullptr, status, 0);
            } else {
                throw err;
            }
        }

        // Perhaps the error handling should be different from the above.
        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            status = backend::sqlite3::result::FAILURE;

            error err {
                  errc::sql_error
                , backend::sqlite3::build_errstr(rc, _rep.sth)
                , backend::sqlite3::current_sql(_rep.sth)
            };

            if (perr) {
                *perr = std::move(err);
                result_type::make(nullptr, status, 0);
            } else {
                throw err;
            }
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_rep.sth);

    return result_type::make(_rep.sth, status, 0);
}

template <>
bool
statement<BACKEND>::bind (int index, std::string const & value
    , transient_enum transient, error * perr)
{
    auto str = value.c_str();
    auto len = value.size();

    return backend::sqlite3::bind_helper_func(& _rep, index, [this, str, len, transient] (int index) {
        return sqlite3_bind_text(_rep.sth, index, str
            , static_cast<int>(len)
            , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }, perr);
}

template <>
bool
statement<BACKEND>::bind (std::string const & placeholder
    , std::string const & value, transient_enum transient, error * perr)
{
    auto str = value.c_str();
    auto len = value.size();

    return backend::sqlite3::bind_helper_func(& _rep, placeholder, [this, str, len, transient] (int index) {
        return sqlite3_bind_text(_rep.sth, index, str
            , static_cast<int>(len)
            , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }, perr);
}

template <>
bool
statement<BACKEND>::bind (int index, string_view value, transient_enum transient
    , error * perr)
{
    auto str = value.data();
    auto len = value.size();

    return backend::sqlite3::bind_helper_func(& _rep, index, [this, str, len, transient] (int index) {
        return sqlite3_bind_text(_rep.sth, index, str
            , static_cast<int>(len)
            , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }, perr);
}

template <>
bool
statement<BACKEND>::bind (std::string const & placeholder, string_view value
    , transient_enum transient, error * perr)
{
    auto str = value.data();
    auto len = value.size();

    return backend::sqlite3::bind_helper_func(& _rep, placeholder, [this, str, len, transient] (int index) {
        return sqlite3_bind_text(_rep.sth, index, str
            , static_cast<int>(len)
            , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }, perr);
}

template <>
bool
statement<BACKEND>::bind (int index, char const * value
    , transient_enum transient, error * perr)
{
    return backend::sqlite3::bind_helper_func(& _rep, index, [this, value, transient] (int index) {
        return sqlite3_bind_text(_rep.sth, index, value
            , static_cast<int>(std::strlen(value))
            , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }, perr);
}

template <>
bool
statement<BACKEND>::bind (std::string const & placeholder
    , char const * value, transient_enum transient, error * perr)
{
    return backend::sqlite3::bind_helper_func(& _rep, placeholder, [this, value, transient] (int index) {
        return sqlite3_bind_text(_rep.sth, index, value
            , static_cast<int>(std::strlen(value))
            , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
    }, perr);
}

template <>
bool
statement<BACKEND>::bind (std::string const & placeholder
    , std::uint8_t const * value, std::size_t len, transient_enum transient
    , error * perr)
{
    if (len > (std::numeric_limits<int>::max)()) {
        return backend::sqlite3::bind_helper_func(& _rep, placeholder, [this, value, len, transient] (int index) {
            return sqlite3_bind_blob64(_rep.sth, index, value
                , static_cast<sqlite3_int64>(len)
                , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
        }, perr);
    } else {
        return backend::sqlite3::bind_helper_func(& _rep, placeholder, [this, value, len, transient] (int index) {
            return sqlite3_bind_blob(_rep.sth, index, value
                , static_cast<int>(len)
                , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
        }, perr);
    }
}

template <>
bool
statement<BACKEND>::bind (int index, std::uint8_t const * value, std::size_t len
    , transient_enum transient, error * perr)
{
    if (len > (std::numeric_limits<int>::max)()) {
        return backend::sqlite3::bind_helper_func(& _rep, index, [this, value, len, transient] (int index) {
            return sqlite3_bind_blob64(_rep.sth, index, value
                , static_cast<sqlite3_int64>(len)
                , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
        }, perr);
    } else {
        return backend::sqlite3::bind_helper_func(& _rep, index, [this, value, len, transient] (int index) {
            return sqlite3_bind_blob(_rep.sth, index, value
                , static_cast<int>(len)
                , transient == transient_enum::yes ? SQLITE_TRANSIENT : SQLITE_STATIC);
        }, perr);
    }
}

} // namespace debby

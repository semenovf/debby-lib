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
//      2024.10.30 V2 implemented.
////////////////////////////////////////////////////////////////////////////////
#include "result_impl.hpp"
#include "statement_impl.hpp"
#include "utils.hpp"
#include <pfs/numeric_cast.hpp>
#include <limits>

DEBBY__NAMESPACE_BEGIN

statement_t::result_type statement_t::impl::exec (error * perr)
{
    std::error_code ec;
    result_t::impl::status status {result_t::impl::INITIAL};
    int rc = sqlite3_step(_sth);

    switch (rc) {
        case SQLITE_ROW:
            status = result_t::impl::ROW;
            break;

        case SQLITE_DONE:
            status = result_t::impl::DONE;
            break;

        case SQLITE_CONSTRAINT:
        case SQLITE_ERROR: {
            status = result_t::impl::FAILURE;

            pfs::throw_or(perr, error{
                  errc::sql_error
                , sqlite3::build_errstr(rc, _sth)
                , sqlite3::current_sql(_sth)
            });

            return result_t{};
        }

        // Perhaps the error handling should be different from the above.
        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            status = result_t::impl::FAILURE;

            pfs::throw_or(perr, error{
                  errc::sql_error
                , sqlite3::build_errstr(rc, _sth)
                , sqlite3::current_sql(_sth)
            });

            return result_t{};
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_sth);

    return result_t{result_t::impl{_sth, status}};
}

template <>
statement_t::statement () {}

template <>
statement_t::statement (impl && d)
{
    _d = new impl(std::move(d));
}

template <>
statement_t::statement (statement && other) noexcept
{
    _d = other._d;
    other._d = nullptr;
}

template <>
statement_t::~statement ()
{
    if (_d != nullptr)
        delete _d;

    _d = nullptr;
}

#define BIND_HELPER_BOILERPLATE_ON_FAILURE          \
        pfs::throw_or(perr, error {                \
              errc::backend_error                  \
            , sqlite3::build_errstr(rc, sth)       \
            , sqlite3::current_sql(sth)            \
        });

#define BIND_HELPER_BOILERPLATE_ON_INVALID_ARG                  \
        pfs::throw_or(perr, error {                             \
              std::make_error_code(std::errc::invalid_argument) \
            , std::string{"bad bind parameter name"}            \
            , placeholder                                       \
        });

template <>
bool statement_t::bind_helper (int index, std::int64_t value, error * perr)
{
    auto sth = _d->native();

    // In sqlite3 index must be started from 1
    auto rc = sqlite3_bind_int64(sth, index + 1, static_cast<sqlite3_int64>(value));

    if (SQLITE_OK != rc) {
        BIND_HELPER_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

template <>
bool statement_t::bind_helper (int index, double value, error * perr)
{
    auto sth = _d->native();

    // In sqlite3 index must be started from 1
    auto rc = sqlite3_bind_double(sth, index + 1, value);

    if (SQLITE_OK != rc) {
        BIND_HELPER_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

template <>
bool statement_t::bind_helper (char const * placeholder, std::int64_t value, error * perr)
{
    auto sth = _d->native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_HELPER_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind_helper(index - 1, value, perr);
}

template <>
bool statement_t::bind_helper (char const * placeholder, double value, error * perr)
{
    auto sth = _d->native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_HELPER_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind_helper(index - 1, value, perr);
}

template <>
bool statement_t::bind (int index, std::nullptr_t, error * perr)
{
    auto sth = _d->native();
    auto rc = sqlite3_bind_null(sth, index + 1);

    if (SQLITE_OK != rc) {
        BIND_HELPER_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

template <>
bool statement_t::bind (char const * placeholder, std::nullptr_t, error * perr)
{
    auto sth = _d->native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_HELPER_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind(index - 1, nullptr, perr);
}

template <>
bool statement_t::bind (int index, std::string && value, error * perr)
{
    auto sth = _d->native();
    auto rc = sqlite3_bind_text(sth, index + 1, value.c_str(), pfs::numeric_cast<int>(value.size()), SQLITE_TRANSIENT);

    if (SQLITE_OK != rc) {
        BIND_HELPER_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

template <>
bool statement_t::bind (char const * placeholder, std::string && value, error * perr)
{
    auto sth = _d->native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_HELPER_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind(index - 1, std::move(value), perr);
}

template <>
bool statement_t::bind (int index, char const * ptr, std::size_t len, error * perr)
{
    auto sth = _d->native();
    int rc = SQLITE_OK;

    if (len > (std::numeric_limits<int>::max)())
        rc = sqlite3_bind_blob64(sth, index + 1, ptr, static_cast<sqlite3_int64>(len), SQLITE_STATIC);
    else
        rc = sqlite3_bind_blob(sth, index + 1, ptr, static_cast<int>(len), SQLITE_STATIC);

    if (SQLITE_OK != rc) {
        BIND_HELPER_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

template <>
bool statement_t::bind (char const * placeholder, char const * ptr, std::size_t len, error * perr)
{
    auto sth = _d->native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_HELPER_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind(index - 1, ptr, len, perr);
}

template <>
statement_t::result_type statement_t::exec (error * perr)
{
    return _d->exec(perr);
}

DEBBY__NAMESPACE_END

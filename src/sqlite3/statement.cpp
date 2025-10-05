////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2024.10.29 V2 started.
//      2024.10.30 V2 implemented.
//      2025.09.30 Changed bind implementation.
////////////////////////////////////////////////////////////////////////////////
#include "result_impl.hpp"
#include "statement_impl.hpp"
#include "utils.hpp"
#include <pfs/i18n.hpp>
#include <pfs/numeric_cast.hpp>
#include <limits>

DEBBY__NAMESPACE_BEGIN

void statement_t::impl::reset (error * perr)
{
    if (_sth != nullptr) {
        int rc = sqlite3_clear_bindings(_sth);

        if (SQLITE_OK != rc) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("clear prepared statement bindings failure: {}", sqlite3::build_errstr(rc, _sth)));
            return;
        }

        rc = sqlite3_reset(_sth);

        if (!(rc == SQLITE_OK || rc == SQLITE_ROW)) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("resetting prepared statement failure: {}", sqlite3::build_errstr(rc, _sth)));
            return;
        }
    }
}

statement_t::result_type statement_t::impl::exec (bool move_handle_ownership, error * perr)
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

            pfs::throw_or(perr, make_error_code(errc::sql_error)
                , fmt::format("{}: {}", sqlite3::build_errstr(rc, _sth), sqlite3::current_sql(_sth)));

            return result_t{};
        }

        // Perhaps the error handling should be different from the above.
        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            status = result_t::impl::FAILURE;

            pfs::throw_or(perr, make_error_code(errc::sql_error)
                , fmt::format("{}: {}", sqlite3::build_errstr(rc, _sth), sqlite3::current_sql(_sth)));

            return result_t{};
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_sth);

    return result_t{result_t::impl{_sth, status, move_handle_ownership}};
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

template <>
statement_t & statement_t::operator = (statement && other)
{
    if (this != & other) {
        this->~statement();
        _d = other._d;
        other._d = nullptr;
    }

    return *this;
}

#define BIND_BOILERPLATE_ON_FAILURE                \
        pfs::throw_or(perr                         \
            , make_error_code(errc::backend_error) \
            , fmt::format("{}: {}", sqlite3::build_errstr(rc, sth), sqlite3::current_sql(sth)));

#define BIND_BOILERPLATE_ON_INVALID_ARG                         \
        pfs::throw_or(perr                                      \
            , std::make_error_code(std::errc::invalid_argument) \
            , tr::f_("bad bind parameter name: {}", placeholder));

bool statement_t::impl::bind_int64 (int index, std::int64_t value, error * perr)
{
    auto sth = native();

    // In sqlite3 index must be started from 1
    auto rc = sqlite3_bind_int64(sth, index, static_cast<sqlite3_int64>(value));

    if (SQLITE_OK != rc) {
        BIND_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

bool statement_t::impl::bind_int64 (char const * placeholder, std::int64_t value, error * perr)
{
    auto sth = native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind_int64(index, value, perr);
}

bool statement_t::impl::bind_double (int index, double value, error * perr)
{
    auto sth = native();

    // In sqlite3 index must be started from 1
    auto rc = sqlite3_bind_double(sth, index, value);

    if (SQLITE_OK != rc) {
        BIND_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

bool statement_t::impl::bind_double (char const * placeholder, double value, error * perr)
{
    auto sth = native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind_double(index, value, perr);
}

bool statement_t::impl::bind_string (int index, char const * ptr, std::size_t len, error * perr)
{
    auto sth = native();
    auto rc = sqlite3_bind_text(sth, index, ptr, pfs::numeric_cast<int>(len), SQLITE_TRANSIENT);

    if (SQLITE_OK != rc) {
        BIND_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

bool statement_t::impl::bind_string (char const * placeholder, char const * ptr, std::size_t len
    , error * perr)
{
    auto sth = native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind_string(index, ptr, len, perr);
}

bool statement_t::impl::bind_blob (int index, char const * ptr, std::size_t len, error * perr)
{
    auto sth = native();
    int rc = SQLITE_OK;

    if (len > (std::numeric_limits<int>::max)())
        rc = sqlite3_bind_blob64(sth, index, ptr, static_cast<sqlite3_int64>(len), SQLITE_STATIC);
    else
        rc = sqlite3_bind_blob(sth, index, ptr, static_cast<int>(len), SQLITE_STATIC);

    if (SQLITE_OK != rc) {
        BIND_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

bool statement_t::impl::bind_blob (char const * placeholder, char const * ptr, std::size_t len
    , error * perr)
{
    auto sth = native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind_blob(index, ptr, len, perr);
}

bool statement_t::impl::bind_null (int index, std::nullptr_t, error * perr)
{
    auto sth = native();
    auto rc = sqlite3_bind_null(sth, index);

    if (SQLITE_OK != rc) {
        BIND_BOILERPLATE_ON_FAILURE
        return false;
    }

    return true;
}

bool statement_t::impl::bind_null (char const * placeholder, std::nullptr_t, error * perr)
{
    auto sth = native();
    int index = sqlite3_bind_parameter_index(sth, placeholder);

    if (index == 0) {
        BIND_BOILERPLATE_ON_INVALID_ARG
        return false;
    }

    return bind_null(index, nullptr, perr);
}

#define DEBBY__INTEGRAL_BIND(t,itype)                              \
    template <>                                                    \
    template <>                                                    \
    bool statement_t::bind<t> (itype index, t const & value, error * perr) \
    {                                                              \
        return _d->bind_int64(index, value, perr);                 \
    }

#define DEBBY__FLOATING_POINT_BIND(t,itype)                        \
    template <>                                                    \
    template <>                                                    \
    bool statement_t::bind<t> (itype index, t const & value, error * perr) \
    {                                                              \
        return _d->bind_double(index, value, perr);                \
    }

DEBBY__INTEGRAL_BIND(bool, int)
DEBBY__INTEGRAL_BIND(char, int)
DEBBY__INTEGRAL_BIND(signed char, int)
DEBBY__INTEGRAL_BIND(unsigned char, int)
DEBBY__INTEGRAL_BIND(short, int)
DEBBY__INTEGRAL_BIND(unsigned short, int)
DEBBY__INTEGRAL_BIND(int, int)
DEBBY__INTEGRAL_BIND(unsigned int, int)
DEBBY__INTEGRAL_BIND(long, int)
DEBBY__INTEGRAL_BIND(unsigned long, int)
DEBBY__INTEGRAL_BIND(long long, int)
DEBBY__INTEGRAL_BIND(unsigned long long, int)

DEBBY__INTEGRAL_BIND(bool, char const *)
DEBBY__INTEGRAL_BIND(char, char const *)
DEBBY__INTEGRAL_BIND(signed char, char const *)
DEBBY__INTEGRAL_BIND(unsigned char, char const *)
DEBBY__INTEGRAL_BIND(short, char const *)
DEBBY__INTEGRAL_BIND(unsigned short, char const *)
DEBBY__INTEGRAL_BIND(int, char const *)
DEBBY__INTEGRAL_BIND(unsigned int, char const *)
DEBBY__INTEGRAL_BIND(long, char const *)
DEBBY__INTEGRAL_BIND(unsigned long, char const *)
DEBBY__INTEGRAL_BIND(long long, char const *)
DEBBY__INTEGRAL_BIND(unsigned long long, char const *)

DEBBY__FLOATING_POINT_BIND(float, int)
DEBBY__FLOATING_POINT_BIND(double, int)
DEBBY__FLOATING_POINT_BIND(float, char const *)
DEBBY__FLOATING_POINT_BIND(double, char const *)

template <>
template <>
bool statement_t::bind<std::string> (int index, std::string const & value, error * perr)
{
    return _d->bind_string(index, value.c_str(), value.size(), perr);
}

template <>
template <>
bool statement_t::bind<std::string> (char const * placeholder, std::string const & value
    , error * perr)
{
    return _d->bind_string(placeholder, value.c_str(), value.size(), perr);
}

template <>
bool statement_t::bind (int index, std::nullptr_t, error * perr)
{
    return _d->bind_null(index, nullptr, perr);
}

template <>
bool statement_t::bind (char const * placeholder, std::nullptr_t, error * perr)
{
    return _d->bind_null(placeholder, nullptr, perr);
}

template <>
bool statement_t::bind (int index, char const * ptr, error * perr)
{
    return _d->bind_string(index, ptr, std::strlen(ptr), perr);
}

template <>
bool statement_t::bind (char const * placeholder, char const * ptr, error * perr)
{
    return _d->bind_string(placeholder, ptr, std::strlen(ptr), perr);
}

template <>
bool statement_t::bind (int index, char const * ptr, std::size_t len, error * perr)
{
    return _d->bind_blob(index, ptr, len, perr);
}

template <>
bool statement_t::bind (char const * placeholder, char const * ptr, std::size_t len, error * perr)
{
    return _d->bind_blob(placeholder, ptr, len, perr);
}

template <>
statement_t::result_type statement_t::exec (error * perr)
{
    return _d->exec(false, perr);
}

template <>
void statement_t::reset (error * perr)
{
    _d->reset(perr);
}

DEBBY__NAMESPACE_END

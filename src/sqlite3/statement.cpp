////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3.h"
#include "utils.hpp"
#include "pfs/bits/compiler.h"
#include "pfs/fmt.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include <cstring>
#include <cassert>

namespace debby {
namespace sqlite3 {

inline std::string statement::current_sql () const noexcept
{
    assert(_sth);
    return sqlite3::current_sql(_sth);
}

int statement::rows_affected_impl () const
{
    assert(_sth);
    auto dbh = sqlite3_db_handle(_sth);
    assert(dbh);

    //return sqlite3_changes64(_sth);
    return sqlite3_changes(dbh);
}

void statement::clear () noexcept
{
    if (_sth) {
        if (!_cached)
            sqlite3_finalize(_sth);
        else
            sqlite3_reset(_sth);
    }

    _sth = nullptr;
}

statement::result_type statement::exec_impl (error * perr)
{
    assert(_sth);

    std::error_code ec;
    result_type::status status {result_type::INITIAL};
    int rc = sqlite3_step(_sth);

    switch (rc) {
        case SQLITE_ROW:
            status = result_type::ROW;
            break;

        case SQLITE_DONE:
            status = result_type::DONE;
            break;

        case SQLITE_CONSTRAINT:
        case SQLITE_ERROR: {
            status = result_type::ERROR;
            ec = make_error_code(errc::sql_error);
            auto err = error{ec, build_errstr(rc, _sth), current_sql()};
            if (perr) *perr = err; else DEBBY__THROW(err);
            break;
        }

        // Perhaps the error handling should be different from the above.
        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            status = result_type::ERROR;
            ec = make_error_code(errc::sql_error);
            auto err = error{ec, build_errstr(rc, _sth), current_sql()};
            if (perr) *perr = err; else DEBBY__THROW(err);
            break;
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_sth);

    return ec ? result_type{nullptr, status, rc} : result_type{_sth, status, 0};
}

bool statement::bind_helper (std::string const & placeholder
    , std::function<int (int /*index*/)> && sqlite3_binder_func
    , error * perr)
{
    assert(_sth);

    int index = sqlite3_bind_parameter_index(_sth, placeholder.c_str());

    if (index == 0) {
        auto ec = make_error_code(errc::invalid_argument);
        auto err = error{ec, std::string{"bad bind parameter name"}
            , placeholder};
        if (perr) *perr = err; else DEBBY__THROW(err);
        return false;
    }

    int rc = sqlite3_binder_func(index);

    if (SQLITE_OK != rc) {
        auto ec = make_error_code(errc::backend_error);
        auto err = error{ec, build_errstr(rc, _sth), current_sql()};
        if (perr) *perr = err; else DEBBY__THROW(err);

        return false;
    }

    return true;
}

bool statement::bind_impl (std::string const & placeholder
    , std::nullptr_t
    , error * perr)
{
    return bind_helper(placeholder, [this] (int index) {
        return sqlite3_bind_null(_sth, index);
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , bool value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, (value ? 1 : 0));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , std::int8_t value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , std::uint8_t value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , std::int16_t value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , std::uint16_t value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , std::int32_t value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , std::uint32_t value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , std::int64_t value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int64(_sth, index, static_cast<sqlite3_int64>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , std::uint64_t value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int64(_sth, index, static_cast<sqlite3_int64>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , float value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_double(_sth, index, static_cast<double>(value));
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , double value
    , error * perr)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_double(_sth, index, value);
    }, perr);
}

bool statement::bind_impl (std::string const & placeholder
    , char const * value, std::size_t len
    , bool static_value
    , error * perr)
{
    if (value == nullptr) {
        return bind_impl(placeholder, nullptr, perr);
    } else {
        return bind_helper(placeholder, [this, value, len, static_value] (int index) {
            return sqlite3_bind_text(_sth, index, value
                , static_cast<int>(len)
                , static_value ? SQLITE_STATIC : SQLITE_TRANSIENT);
        }, perr);
    }
}

bool statement::bind_impl (std::string const & placeholder
    , std::vector<std::uint8_t> const & value
    , bool static_value
    , error * perr)
{
    if (value.size() > std::numeric_limits<int>::max()) {
        auto data = value.data();
        sqlite3_uint64 len = value.size();

        return bind_helper(placeholder, [this, data, len, static_value] (int index) {
            return sqlite3_bind_blob64(_sth, index, data, len
                , static_value ? SQLITE_STATIC : SQLITE_TRANSIENT);
        }, perr);
    } else {
        auto data = value.data();
        int len = static_cast<int>(value.size());

        return bind_helper(placeholder, [this, data, len,  static_value] (int index) {
            return sqlite3_bind_blob(_sth, index, data, len
                , static_value ? SQLITE_STATIC : SQLITE_TRANSIENT
            );
        }, perr);
    }
}

}} // namespace debby::sqlite3

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
////////////////////////////////////////////////////////////////////////////////
#include "result_impl.hpp"
#include "utils.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <cstring>
#include <utility>

DEBBY__NAMESPACE_BEGIN

template <>
result_t::result () {}

template <>
result_t::result (impl && d)
{
    _d = new impl(std::move(d));
}

template <>
result_t::result (result && other) noexcept
{
    _d = other._d;
    other._d = nullptr;
}

template <>
result_t::~result ()
{
    if (_d != nullptr)
        delete _d;

    _d = nullptr;
}

template <>
int result_t::rows_affected () const
{
    auto dbh = sqlite3_db_handle(_d->sth);

    PFS__TERMINATE(dbh != nullptr, "");

    //return sqlite3_changes64(_sth);
    return sqlite3_changes(dbh);
}

template <>
bool result_t::has_more () const noexcept
{
    return _d->state == impl::ROW;
}

template <>
bool result_t::is_done () const noexcept
{
    return _d->state == impl::DONE;
}

template <>
bool result_t::is_error () const noexcept
{
    return _d->state == impl::FAILURE;
}

template <>
int result_t::column_count () const noexcept
{
    return sqlite3_column_count(_d->sth);
}

template <>
std::string result_t::column_name (int column) const noexcept
{
    if (column >= 0 && column < sqlite3_column_count(_d->sth))
        return std::string {sqlite3_column_name(_d->sth, column)};

    return std::string{};
}

template <>
void result_t::next ()
{
    auto rc = sqlite3_step(_d->sth);

    switch (rc) {
        case SQLITE_ROW:
            _d->state = impl::ROW;
            break;

        case SQLITE_DONE:
            _d->state = impl::DONE;
            break;

        case SQLITE_CONSTRAINT:
        case SQLITE_ERROR: {
            _d->state = impl::FAILURE;

            throw error {
                  errc::sql_error
                , sqlite3::build_errstr(rc, _d->sth)
                , sqlite3::current_sql(_d->sth)
            };

            break;
        }

        // Perhaps the error handling should be different from the above.
        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            _d->state = impl::FAILURE;

            throw error {
                  errc::sql_error
                , sqlite3::build_errstr(rc, _d->sth)
                , sqlite3::current_sql(_d->sth)
            };

            break;
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_d->sth);
}

inline char * ensure_capacity (char * buffer, std::size_t size, std::size_t requred_size)
{
    if (size < requred_size) {
        buffer = new char[requred_size];
        return buffer;
    }

    return buffer;
}

template <>
std::pair<char *, std::size_t>
result_t::fetch (int column, char * buffer, std::size_t initial_size, error & err) const
{
    auto upper_limit = sqlite3_column_count(_d->sth);

    if (column < 0 || column >= upper_limit) {
        err = error {
              errc::column_not_found
            , tr::f_("bad column: {}, expected greater or equal to 0 and"
                " less than {}", column, upper_limit)
        };

        return std::make_pair(static_cast<char *>(nullptr), std::size_t{0});
    }

    auto column_type = sqlite3_column_type(_d->sth, column);

    switch (column_type) {
        case SQLITE_INTEGER: {
            sqlite3_int64 n = sqlite3_column_int64(_d->sth, column);

            buffer = ensure_capacity(buffer, initial_size, sizeof(n));
            std::memcpy(buffer, & n, sizeof(n));
            return std::make_pair(buffer, sizeof(n));
        }

        case SQLITE_FLOAT: {
            double f = sqlite3_column_double(_d->sth, column);

            buffer = ensure_capacity(buffer, initial_size, sizeof(f));
            std::memcpy(buffer, & f, sizeof(f));
            return std::make_pair(buffer, sizeof(f));
        }

        case SQLITE_TEXT: {
            auto cstr = reinterpret_cast<char const *>(sqlite3_column_text(_d->sth, column));
            int size = sqlite3_column_bytes(_d->sth, column);

            buffer = ensure_capacity(buffer, initial_size, size);
            std::memcpy(buffer, cstr, size);
            return std::make_pair(buffer, size);
        }

        case SQLITE_BLOB: {
            auto data = static_cast<char const *>(sqlite3_column_blob(_d->sth, column));
            int size = sqlite3_column_bytes(_d->sth, column);

            buffer = ensure_capacity(buffer, initial_size, size);
            std::memcpy(buffer, data, size);
            return std::make_pair(buffer, size);
        }

        case SQLITE_NULL:
            return std::make_pair(static_cast<char *>(nullptr), std::size_t{0});

        default:
            // Unexpected column type, need to handle it.
            PFS__TERMINATE(false, "Unexpected column type");
            break;
    }

    // Unreachable in ordinary situation
    return std::make_pair(static_cast<char *>(nullptr), std::size_t{0});
}

template <>
std::pair<char *, std::size_t>
result_t::fetch (std::string const & column_name, char * buffer, std::size_t initial_size, error & err) const
{
    if (_d->column_mapping.empty()) {
        auto count = sqlite3_column_count(_d->sth);

        for (int i = 0; i < count; i++)
            _d->column_mapping.insert({std::string{sqlite3_column_name(_d->sth, i)}, i});
    }

    auto pos = _d->column_mapping.find(column_name);

    if (pos == _d->column_mapping.end()) {
        err = error {
            errc::column_not_found
            , tr::f_("bad column name: {}", column_name)
        };

        return std::make_pair(static_cast<char *>(nullptr), std::size_t{0});
    }

    return fetch(pos->second, buffer, initial_size, err);
}

DEBBY__NAMESPACE_END

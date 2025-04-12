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
#include "../fixed_packer.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <cstring>
#include <utility>

DEBBY__NAMESPACE_BEGIN

template <>
result_t::result () {}

template <>
result_t::result (impl && d) noexcept
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
                  make_error_code(errc::sql_error)
                , fmt::format("{}: {}", sqlite3::build_errstr(rc, _d->sth)
                    , sqlite3::current_sql(_d->sth))
            };

            break;
        }

        // Perhaps the error handling should be different from the above.
        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            _d->state = impl::FAILURE;

            throw error {
                  make_error_code(errc::sql_error)
                , fmt::format("{} :{}", sqlite3::build_errstr(rc, _d->sth)
                    , sqlite3::current_sql(_d->sth))
            };

            break;
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_d->sth);
}

#define CHECK_COLUMN_INDEX_BOILERPLATE                                          \
    if (column < 0 || column >= _d->column_count) {                             \
        pfs::throw_or(perr, error {                                             \
              make_error_code(errc::column_not_found)                           \
            , tr::f_("bad column index: {}, expected greater or equal to 0 and" \
                " less than {}", column, _d->column_count)                      \
        });                                                                     \
        return pfs::nullopt;                                                    \
    }

#define UNSUITABLE_ERROR_BOILERPLATE \
    pfs::throw_or(perr, error {make_error_code(errc::bad_value)                 \
        , tr::f_("unsuitable column type at index {}", column)});

template <>
pfs::optional<std::int64_t> result_t::get_int64 (int column, error * perr)
{
    CHECK_COLUMN_INDEX_BOILERPLATE

    auto column_type = sqlite3_column_type(_d->sth, column);

    switch (column_type) {
        case SQLITE_INTEGER:
            return sqlite3_column_int64(_d->sth, column);
        case SQLITE_NULL:
            return pfs::nullopt;
        case SQLITE_BLOB: { // Used by key/value database
            auto bytes = static_cast<char const *>(sqlite3_column_blob(_d->sth, column));
            int size = sqlite3_column_bytes(_d->sth, column);
            fixed_packer<std::int64_t> fx;

            if (size != sizeof(fx.bytes))
                break;

            std::memcpy(fx.bytes, bytes, size);
            return fx.value;
        }
        default:
            break;
    }

    UNSUITABLE_ERROR_BOILERPLATE
    return pfs::nullopt;
}

template <>
pfs::optional<double> result_t::get_double (int column, error * perr)
{
    CHECK_COLUMN_INDEX_BOILERPLATE

    auto column_type = sqlite3_column_type(_d->sth, column);

    switch (column_type) {
        case SQLITE_FLOAT:
            return sqlite3_column_double(_d->sth, column);
        case SQLITE_NULL:
            return pfs::nullopt;
        case SQLITE_BLOB: { // Used by key/value database
            auto bytes = static_cast<char const *>(sqlite3_column_blob(_d->sth, column));
            int size = sqlite3_column_bytes(_d->sth, column);
            fixed_packer<double> fx;

            if (size != sizeof(fx.bytes))
                break;

            std::memcpy(fx.bytes, bytes, size);
            return fx.value;
        }
        default:
            break;
    }

    UNSUITABLE_ERROR_BOILERPLATE
    return pfs::nullopt;
}

template <>
pfs::optional<std::string> result_t::get_string (int column, error * perr)
{
    CHECK_COLUMN_INDEX_BOILERPLATE

    auto column_type = sqlite3_column_type(_d->sth, column);

    switch (column_type) {
        case SQLITE_INTEGER:
            return std::to_string(sqlite3_column_int64(_d->sth, column));
        case SQLITE_FLOAT:
            return std::to_string(sqlite3_column_double(_d->sth, column));
        case SQLITE_TEXT: {
            auto chars = reinterpret_cast<char const *>(sqlite3_column_text(_d->sth, column));
            int size = sqlite3_column_bytes(_d->sth, column);
            return std::string(chars, size);
        }
        case SQLITE_BLOB: {
            auto bytes = static_cast<char const *>(sqlite3_column_blob(_d->sth, column));
            int size = sqlite3_column_bytes(_d->sth, column);
            return std::string(bytes, size);
        }
        case SQLITE_NULL:
            return pfs::nullopt;
        default:
            break;
    }

    UNSUITABLE_ERROR_BOILERPLATE
    return pfs::nullopt;
}

template <>
pfs::optional<std::int64_t> result_t::get_int64 (std::string const & column_name, error * perr)
{
    auto index = _d->column_index(column_name);

    if (index < 0) {
        pfs::throw_or(perr, make_error_code(errc::column_not_found), tr::f_("bad column name: {}", column_name));
        return 0;
    }

    return get_int64(index, perr);
}

template <>
pfs::optional<double> result_t::get_double (std::string const & column_name, error * perr)
{
    auto index = _d->column_index(column_name);

    if (index < 0) {
        pfs::throw_or(perr, make_error_code(errc::column_not_found), tr::f_("bad column name: {}", column_name));
        return 0;
    }

    return get_double(index, perr);
}

template <>
pfs::optional<std::string> result_t::get_string (std::string const & column_name, error * perr)
{
    auto index = _d->column_index(column_name);

    if (index < 0) {
        pfs::throw_or(perr, make_error_code(errc::column_not_found), tr::f_("bad column name: {}", column_name));
        return std::string{};
    }

    return get_string(index, perr);
}

DEBBY__NAMESPACE_END

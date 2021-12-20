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
#include "pfs/fmt.hpp"
#include "pfs/debby/sqlite3/result.hpp"
#include <cstring>
#include <cassert>

namespace debby {
namespace sqlite3 {

namespace {
char const * NULL_HANDLER = "uninitialized statement handler";
} // namespace

std::string result::current_sql () const noexcept
{
    DEBBY__ASSERT(_sth, NULL_HANDLER);
    return sqlite3::current_sql(_sth);
}

int result::column_count_impl () const noexcept
{
    DEBBY__ASSERT(_sth, NULL_HANDLER);
    return sqlite3_column_count(_sth);
}

pfs::string_view result::column_name_impl (int column) const noexcept
{
    DEBBY__ASSERT(_sth, NULL_HANDLER);

    if (column >= 0 && column < sqlite3_column_count(_sth))
        return pfs::string_view {sqlite3_column_name(_sth, column)};

    return pfs::string_view{};
}

void result::next_impl (error * perr)
{
    DEBBY__ASSERT(_sth, NULL_HANDLER);

    auto rc = sqlite3_step(_sth);

    switch (rc) {
        case SQLITE_ROW:
            _state = ROW;
            break;

        case SQLITE_DONE:
            _state = DONE;
            break;

        case SQLITE_CONSTRAINT:
        case SQLITE_ERROR: {
            _state = ERROR;
            auto ec = make_error_code(errc::sql_error);
            auto err = error{ec, build_errstr(rc, _sth), current_sql()};
            if (perr) *perr = err; else DEBBY__THROW(err);
            break;
        }

        // Perhaps the error handling should be different from the above.
        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            _state = ERROR;
            auto ec = make_error_code(errc::sql_error);
            auto err = error{ec, build_errstr(rc, _sth), current_sql()};
            if (perr) *perr = err; else DEBBY__THROW(err);
            break;
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_sth);
}

result::value_type result::fetch_impl (int column, error * perr)
{
    DEBBY__ASSERT(_sth, NULL_HANDLER);

    auto upper_limit = sqlite3_column_count(_sth);

    if (column < 0 || column >= upper_limit) {
        _state = ERROR;

        auto ec = make_error_code(errc::invalid_argument);
        auto err = error{ec
            , fmt::format("bad column: {}, expected greater or equal to 0 and"
                " less than {}", column, upper_limit)};
        if (perr) *perr = err; else DEBBY__THROW(err);
        return result::value_type{nullptr};
    }

    auto column_type = sqlite3_column_type(_sth, column);

    switch (column_type) {
        case SQLITE_INTEGER: {
            sqlite3_int64 n = sqlite3_column_int64(_sth, column);
            return result::value_type{static_cast<std::intmax_t>(n)};
        }

        case SQLITE_FLOAT: {
            double f = sqlite3_column_double(_sth, column);
            return result::value_type{f};
        }

        case SQLITE_TEXT: {
            auto cstr = reinterpret_cast<char const *>(sqlite3_column_text(_sth, column));
            int size = sqlite3_column_bytes(_sth, column);
            return result::value_type{std::string(cstr, size)};
        }

        case SQLITE_BLOB: {
            std::uint8_t const * data = static_cast<std::uint8_t const *>(sqlite3_column_blob(_sth, column));
            int size = sqlite3_column_bytes(_sth, column);
            blob_type blob;
            blob.resize(size);
            std::memcpy(blob.data(), data, size);
            return result::value_type{std::move(blob)};
        }

        case SQLITE_NULL:
            return result::value_type{nullptr};

        default:
            // Unexpected column type, need to handle it.
            DEBBY__ASSERT(false, "Unexpected column type");
            break;
    }

    // Unreachable in ordinary situation
    return result::value_type{nullptr};
}

result::value_type result::fetch_impl (pfs::string_view name, error * perr)
{
    DEBBY__ASSERT(_sth, NULL_HANDLER);

    if (_column_mapping.empty()) {
        auto count = sqlite3_column_count(_sth);

        for (int i = 0; i < count; i++)
            _column_mapping.insert({pfs::string_view{sqlite3_column_name(_sth, i)}, i});
    }

    auto pos = _column_mapping.find(name);

    if (pos != _column_mapping.end())
        return fetch_impl(pos->second, perr);

    _state = ERROR;

    auto ec = make_error_code(errc::invalid_argument);
    auto err = error{ec, fmt::format("bad column name: {}", name.to_string())};
    if (perr) *perr = err; else DEBBY__THROW(err);

    return result::value_type{nullptr};
}

}} // namespace debby::sqlite3

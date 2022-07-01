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
#include "pfs/debby/result.hpp"
#include "pfs/debby/backend/sqlite3/result.hpp"

namespace debby {

static char const * NULL_HANDLER = "uninitialized statement handler";

namespace backend {
namespace sqlite3 {

result::rep_type result::make (handle_type sth
    , status state, int error_code)
{
    rep_type rep;
    rep.sth = sth;
    rep.state = state;
    rep.error_code = error_code;

    return rep;
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::result

////////////////////////////////////////////////////////////////////////////////
// result::result
////////////////////////////////////////////////////////////////////////////////
template <>
result<BACKEND>::result (rep_type && rep)
    : _rep(std::move(rep))
{}

////////////////////////////////////////////////////////////////////////////////
// result::result
////////////////////////////////////////////////////////////////////////////////
template <>
result<BACKEND>::result (result && other)
{
    _rep = std::move(other._rep);
    other._rep.sth = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// ~result
////////////////////////////////////////////////////////////////////////////////
template <>
result<BACKEND>::~result ()
{}

////////////////////////////////////////////////////////////////////////////////
// operator bool
////////////////////////////////////////////////////////////////////////////////
template <>
result<BACKEND>::operator bool () const noexcept
{
    return _rep.sth != nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// has_more
////////////////////////////////////////////////////////////////////////////////
template <>
bool
result<BACKEND>::has_more () const noexcept
{
    return _rep.state == backend::sqlite3::result::ROW;
}

////////////////////////////////////////////////////////////////////////////////
// is_done
////////////////////////////////////////////////////////////////////////////////
template <>
bool
result<BACKEND>::is_done () const noexcept
{
    return _rep.state == backend::sqlite3::result::DONE;
}

////////////////////////////////////////////////////////////////////////////////
// is_error
////////////////////////////////////////////////////////////////////////////////
// template <>
// bool
// result<BACKEND>::is_error () const noexcept
// {
//     return _rep.state == backend::sqlite3::result::ERROR;
// }

////////////////////////////////////////////////////////////////////////////////
// column_count
////////////////////////////////////////////////////////////////////////////////
template <>
int
result<BACKEND>::column_count () const noexcept
{
    DEBBY__ASSERT(_rep.sth, NULL_HANDLER);
    return sqlite3_column_count(_rep.sth);
}

////////////////////////////////////////////////////////////////////////////////
// column_name
////////////////////////////////////////////////////////////////////////////////
template <>
std::string
result<BACKEND>::column_name (int column) const noexcept
{
    DEBBY__ASSERT(_rep.sth, NULL_HANDLER);

    if (column >= 0 && column < sqlite3_column_count(_rep.sth))
        return std::string {sqlite3_column_name(_rep.sth, column)};

    return std::string{};
}

////////////////////////////////////////////////////////////////////////////////
// next
////////////////////////////////////////////////////////////////////////////////
template <>
void
result<BACKEND>::next ()
{
    DEBBY__ASSERT(_rep.sth, NULL_HANDLER);

    auto rc = sqlite3_step(_rep.sth);

    switch (rc) {
        case SQLITE_ROW:
            _rep.state = backend::sqlite3::result::ROW;
            break;

        case SQLITE_DONE:
            _rep.state = backend::sqlite3::result::DONE;
            break;

        case SQLITE_CONSTRAINT:
        case SQLITE_ERROR: {
            _rep.state = backend::sqlite3::result::FAILURE;

            auto err = error{
                  make_error_code(errc::sql_error)
                , backend::sqlite3::build_errstr(rc, _rep.sth)
                , backend::sqlite3::current_sql(_rep.sth)
            };

            DEBBY__THROW(err);
            break;
        }

        // Perhaps the error handling should be different from the above.
        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            _rep.state = backend::sqlite3::result::FAILURE;

            auto err = error{
                  make_error_code(errc::sql_error)
                , backend::sqlite3::build_errstr(rc, _rep.sth)
                , backend::sqlite3::current_sql(_rep.sth)
            };

            DEBBY__THROW(err);
            break;
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_rep.sth);
}

////////////////////////////////////////////////////////////////////////////////
// fetch
////////////////////////////////////////////////////////////////////////////////
template <>
result_status
result<BACKEND>::fetch (int column, value_type & value) const noexcept
{
    DEBBY__ASSERT(_rep.sth, NULL_HANDLER);

    auto upper_limit = sqlite3_column_count(_rep.sth);

    if (column < 0 || column >= upper_limit) {
        auto err = error{
              make_error_code(errc::column_not_found)
            , fmt::format("bad column: {}, expected greater or equal to 0 and"
                " less than {}", column, upper_limit)
        };

        return err;
    }

    auto column_type = sqlite3_column_type(_rep.sth, column);

    switch (column_type) {
        case SQLITE_INTEGER: {
            sqlite3_int64 n = sqlite3_column_int64(_rep.sth, column);
            value = result::value_type{static_cast<std::intmax_t>(n)};
            return result_status{};
        }

        case SQLITE_FLOAT: {
            double f = sqlite3_column_double(_rep.sth, column);
            value = result::value_type{f};
            return result_status{};
        }

        case SQLITE_TEXT: {
            auto cstr = reinterpret_cast<char const *>(sqlite3_column_text(_rep.sth, column));
            int size = sqlite3_column_bytes(_rep.sth, column);
            value = result::value_type{std::string(cstr, size)};
            return result_status{};
        }

        case SQLITE_BLOB: {
            char const * data = static_cast<char const *>(sqlite3_column_blob(_rep.sth, column));
            int size = sqlite3_column_bytes(_rep.sth, column);
            blob_t blob;
            blob.resize(size);
            std::memcpy(blob.data(), data, size);
            value = result::value_type{std::move(blob)};
            return result_status{};
        }

        case SQLITE_NULL:
            value = result::value_type{nullptr};
            return result_status{};

        default:
            // Unexpected column type, need to handle it.
            DEBBY__ASSERT(false, "Unexpected column type");
            break;
    }

    // Unreachable in ordinary situation
    return result_status{};
}

////////////////////////////////////////////////////////////////////////////////
// fetch
////////////////////////////////////////////////////////////////////////////////
template <>
result_status
result<BACKEND>::fetch (std::string const & column_name, value_type & value) const noexcept
{
    DEBBY__ASSERT(_rep.sth, NULL_HANDLER);

    if (_rep.column_mapping.empty()) {
        auto count = sqlite3_column_count(_rep.sth);

        for (int i = 0; i < count; i++)
            _rep.column_mapping.insert({std::string{sqlite3_column_name(_rep.sth, i)}, i});
    }

    auto pos = _rep.column_mapping.find(column_name);

    if (pos != _rep.column_mapping.end())
        return fetch(pos->second, value);

    auto err = error{
          make_error_code(errc::column_not_found)
        , fmt::format("bad column name: {}", column_name)
    };

    return err;
}

} // namespace debby

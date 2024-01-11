////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
// #include "utils.hpp"
#include "oid_enum.hpp"
#include "pfs/assert.hpp"
#include "pfs/i18n.hpp"
#include "pfs/integer.hpp"
#include "pfs/debby/result.hpp"
#include "pfs/debby/backend/psql/result.hpp"
#include <cstring>

extern "C" {
#include <libpq-fe.h>
}

namespace debby {

static char const * NULL_HANDLER_TEXT = "uninitialized statement handler";

namespace backend {
namespace psql {

result::rep_type result::make (handle_type sth)
{
    rep_type rep;
    rep.sth = sth;
    rep.row_count = PQntuples(sth);
    rep.row_index = 0;
    return rep;
}

}} // namespace backend::psql

using BACKEND = backend::psql::result;

template <>
result<BACKEND>::result (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
result<BACKEND>::result (result && other)
{
    _rep = std::move(other._rep);
    other._rep.sth = nullptr;
}

template <>
result<BACKEND>::~result ()
{
    if (_rep.sth != nullptr)
        PQclear(_rep.sth);

    _rep.sth = nullptr;
}

template <>
result<BACKEND>::operator bool () const noexcept
{
    return _rep.sth != nullptr;
}

template <>
int
result<BACKEND>::rows_affected () const
{
    PFS__ASSERT(_rep.sth, NULL_HANDLER_TEXT);

    char * str = PQcmdTuples(_rep.sth);

    if (str == nullptr)
        return 0;

    if (str[0] == '\x0')
        return 0;

    std::error_code ec;
    auto n = pfs::to_integer<int>(str, str + std::strlen(str), 10, ec);

    if (ec) {
        throw error {
              errc::backend_error
            , tr::f_("unexpected string value returned by `PQcmdTuples`"
                " represented the number of affected rows: {}", str)
        };
    }

    return n;
}

template <>
bool
result<BACKEND>::has_more () const noexcept
{
    return _rep.row_index < _rep.row_count;
}

template <>
bool
result<BACKEND>::is_done () const noexcept
{
    return _rep.row_count == _rep.row_index;
}

// template <>
// bool
// result<BACKEND>::is_error () const noexcept
// {}

template <>
int
result<BACKEND>::column_count () const noexcept
{
    PFS__ASSERT(_rep.sth, NULL_HANDLER_TEXT);
    return PQnfields(_rep.sth);
}

template <>
std::string
result<BACKEND>::column_name (int column) const noexcept
{
    PFS__ASSERT(_rep.sth, NULL_HANDLER_TEXT);
    auto cname = PQfname(_rep.sth, column);
    return cname == nullptr ? std::string{} : std::string{cname};
}

template <>
void
result<BACKEND>::next ()
{
    PFS__ASSERT(_rep.sth, NULL_HANDLER_TEXT);

    if (_rep.row_index < _rep.row_count) {
        ++_rep.row_index;
    } else {
        throw std::overflow_error("result::next()");
    }
}

template <>
bool
result<BACKEND>::fetch (int column, value_type & value, error & err) const noexcept
{
    PFS__ASSERT(_rep.sth, NULL_HANDLER_TEXT);

    auto upper_limit = PQnfields(_rep.sth);

    if (column < 0 || column >= upper_limit) {
        err = error {
              errc::column_not_found
            , fmt::format("bad column: {}, expected greater or equal to 0 and"
                " less than {}", column, upper_limit)
        };

        return false;
    }

    auto is_null = PQgetisnull(_rep.sth, _rep.row_index, column) != 0;

    if (is_null) {
        if (pfs::holds_alternative<bool>(value)) {
            value = value_type::make_zero<bool>();
        } else if (pfs::holds_alternative<std::intmax_t>(value)) {
            value = value_type::make_zero<std::intmax_t>();
        } else if (pfs::holds_alternative<float>(value)) {
            value = value_type::make_zero<float>();
        } else if (pfs::holds_alternative<double>(value)) {
            value = value_type::make_zero<double>();
        } else if (pfs::holds_alternative<blob_t>(value)) {
            value = value_type::make_zero<blob_t>();
        } else if (pfs::holds_alternative<std::string>(value)) {
            value = value_type::make_zero<std::string>();
        } else { // std::nullptr_t
            value = nullptr;
        }

        return true;
    }

    using namespace backend::psql;
    auto column_type = static_cast<oid_enum>(PQftype(_rep.sth, column));

    auto raw_data = reinterpret_cast<char const *>(PQgetvalue(_rep.sth, _rep.row_index, column));
    int size = PQgetlength(_rep.sth, _rep.row_index, column);

    switch (column_type) {
        case oid_enum::boolean:
        case oid_enum::int16:
        case oid_enum::int32:
        case oid_enum::int64: {
//         case SQLITE_INTEGER: {
//             sqlite3_int64 n = sqlite3_column_int64(_rep.sth, column);
//
//             if (pfs::holds_alternative<bool>(value)) {
//                 value = static_cast<bool>(n);
//             } else if (pfs::holds_alternative<double>(value)) {
//                 value = static_cast<double>(n);
//             } else if (pfs::holds_alternative<float>(value)) {
//                 value = static_cast<float>(n);
//             } else if (pfs::holds_alternative<blob_t>(value)) {
//                 blob_t blob(sizeof(sqlite3_int64));
//                 std::memcpy(blob.data(), &n, sizeof(n));
//                 value = std::move(blob);
//             } else if (pfs::holds_alternative<std::string>(value)) {
//                 std::string s = std::to_string(n);
//                 value = std::move(s);
//             } else { // std::nullptr_t, std::intmax_t
//                 value = static_cast<std::intmax_t>(n);
//             }

            return true;
        }

//         case SQLITE_FLOAT: {
//             double f = sqlite3_column_double(_rep.sth, column);
//
//             if (pfs::holds_alternative<double>(value)) {
//                 value = f;
//             } else if (pfs::holds_alternative<float>(value)) {
//                 value = static_cast<float>(f);
//             } else if (pfs::holds_alternative<bool>(value)) {
//                 value = static_cast<bool>(f);
//             } else if (pfs::holds_alternative<std::intmax_t>(value)) {
//                 value = static_cast<std::intmax_t>(f);
//             } else if (pfs::holds_alternative<blob_t>(value)) {
//                 blob_t blob(sizeof(double));
//                 std::memcpy(blob.data(), & f, sizeof(f));
//                 value = std::move(blob);
//             } else if (pfs::holds_alternative<std::string>(value)) {
//                 std::string s = std::to_string(f);
//                 value = std::move(s);
//             } else { // std::nullptr_t
//                 value = f;
//             }
//
//             return true;
//         }
//
        case oid_enum::name: {
            value = std::string(raw_data, size);
            return true;
        }

//         case SQLITE_BLOB: {
//             char const * data = static_cast<char const *>(sqlite3_column_blob(_rep.sth, column));
//             int size = sqlite3_column_bytes(_rep.sth, column);
//             blob_t blob;
//             blob.resize(size);
//             std::memcpy(blob.data(), data, size);
//             value = std::move(blob);
//             return true;
//         }
//         default:
//             // Unexpected column type, need to handle it.
//             PFS__TERMINATE(false, "Unexpected column type");
//             break;
    }

    // Unreachable in ordinary situation
    return true;
}

template <>
bool
result<BACKEND>::fetch (std::string const & column_name, value_type & value
    , error & err) const noexcept
{
    PFS__ASSERT(_rep.sth, NULL_HANDLER_TEXT);

//     if (_rep.column_mapping.empty()) {
//         auto count = sqlite3_column_count(_rep.sth);
//
//         for (int i = 0; i < count; i++)
//             _rep.column_mapping.insert({std::string{sqlite3_column_name(_rep.sth, i)}, i});
//     }
//
//     auto pos = _rep.column_mapping.find(column_name);
//
//     if (pos != _rep.column_mapping.end())
//         return fetch(pos->second, value, err);
//
//     err = error {
//           errc::column_not_found
//         , fmt::format("bad column name: {}", column_name)
//     };

    return false;
}

} // namespace debby

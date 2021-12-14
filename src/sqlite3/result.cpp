////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3.h"
#include "utils.hpp"
#include "pfs/fmt.hpp"
#include "pfs/debby/sqlite3/result.hpp"
#include <cstring>
#include <cassert>

namespace pfs {
namespace debby {
namespace sqlite3 {

std::string const result::ERROR_DOMAIN {"SQLITE3"};

namespace {
    std::string UNITIALIZED_STATEMENT_ERROR {"statement not initialized"};
} // namespace

std::string result::current_sql () const noexcept
{
    assert(_sth);
    return sqlite3::current_sql(_sth);
}

void result::next_impl ()
{
    assert(_sth);

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
            PFS_DEBBY_THROW((sql_error{
                  ERROR_DOMAIN
                , fmt::format("result failure: {}", build_errstr(rc, _sth))
                , current_sql()
            }));
            _state = ERROR;
            break;
        }

        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default:
            PFS_DEBBY_THROW((sql_error{
                  ERROR_DOMAIN
                , fmt::format("result failure: {}", build_errstr(rc, _sth))
                , current_sql()
            }));
            break;
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_sth);
}

int result::column_count_impl () const noexcept
{
    assert(_sth);
    return sqlite3_column_count(_sth);
}

string_view result::column_name_impl (int column) const noexcept
{
    assert(_sth);

    if (column >= 0 && column < sqlite3_column_count(_sth))
        return string_view {sqlite3_column_name(_sth, column)};

    return string_view{};
}

result::value_type result::fetch_impl (int column)
{
    assert(_sth);

    if (column < 0 || column >= sqlite3_column_count(_sth)) {
        _state = ERROR;

        PFS_DEBBY_THROW((invalid_argument{
              ERROR_DOMAIN
            , fmt::format("bad column index: {}", column)
            , current_sql()
        }));

        return result::value_type{};
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
            PFS_DEBBY_THROW((unexpected_error{
                  ERROR_DOMAIN
                , fmt::format("unexpected column type: {}, need to handle it", column_type)
            }));

            break;
    }

    // Unreachable in ordinary situation
    return result::value_type{}; // nullptr
}

result::value_type result::fetch_impl (string_view name)
{
    assert(_sth);

    if (_column_mapping.empty()) {
        auto count = sqlite3_column_count(_sth);

        for (int i = 0; i < count; i++)
            _column_mapping.insert({string_view{sqlite3_column_name(_sth, i)}, i});
    }

    auto pos = _column_mapping.find(name);

    if (pos != _column_mapping.end())
        return fetch_impl(pos->second);

    _state = ERROR;

    PFS_DEBBY_THROW((invalid_argument{
          ERROR_DOMAIN
        , fmt::format("column name is invalid: {}", name.to_string())
        , current_sql()
    }));

    return result::value_type{}; // nullptr
}

}}} // namespace pfs::debby::sqlite3

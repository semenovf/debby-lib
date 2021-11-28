////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/fmt.hpp"
#include "pfs/debby/sqlite3/result.hpp"
#include <cstring>
#include <cassert>

namespace pfs {
namespace debby {
namespace sqlite3 {

PFS_DEBBY__EXPORT void result::next_impl ()
{
    if (!_sth) {
        _last_error = "statement not initialized";
        _state = ERROR;
        return;
    }

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

            auto dbh = sqlite3_db_handle(_sth);

            if (dbh) {
                _last_error = sqlite3_errmsg(dbh);
            } else {
                rc = sqlite3_reset(_sth);
                _last_error = sqlite3_errstr(rc);
            }
            break;
        }

        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default:
            _state = ERROR;
            _last_error = sqlite3_errstr(rc);
            break;
    }
}

PFS_DEBBY__EXPORT int result::column_count_impl () const
{
    assert(_sth);
    return sqlite3_column_count(_sth);
}

PFS_DEBBY__EXPORT std::string result::column_name_impl (int column) const
{
    assert(_sth);
    std::string name;

    if (column >= 0 && column < sqlite3_column_count(_sth)) {
        name = std::string {sqlite3_column_name(_sth, column)};
    }

    return name;
}

PFS_DEBBY__EXPORT optional<result::value_type> result::get_impl (int column)
{
    assert(_sth);

    if (column < 0 || column >= sqlite3_column_count(_sth)) {
        _state = ERROR;
        _last_error = fmt::format("column is out of bounds: {} [0,{})"
            , column, sqlite3_column_count(_sth));
        return optional<result::value_type>{};
    }

    switch (sqlite3_column_type(_sth, column)) {
        case SQLITE_INTEGER: {
            sqlite3_int64 n = sqlite3_column_int64(_sth, column);
            return optional<result::value_type>{static_cast<std::intmax_t>(n)};
        }

        case SQLITE_FLOAT: {
            double f = sqlite3_column_double(_sth, column);
            return optional<result::value_type>{f};
        }

        case SQLITE_TEXT: {
            auto cstr = reinterpret_cast<char const *>(sqlite3_column_text(_sth, column));
            int size = sqlite3_column_bytes(_sth, column);
            return optional<result::value_type>{std::string(cstr, size)};
        }

        case SQLITE_BLOB: {
            std::uint8_t const * data = static_cast<std::uint8_t const *>(sqlite3_column_blob(_sth, column));
            int size = sqlite3_column_bytes(_sth, column);
            blob_type blob;
            blob.resize(size);
            std::memcpy(blob.data(), data, size);
            return optional<result::value_type>{std::move(blob)};
        }

        case SQLITE_NULL:
            return optional<result::value_type>{nullptr};

        default:
            assert(false); // Unexpected column type, need to handle it
            break;
    }

    // Unreachable in ordinary situation
    return optional<result::value_type>{};
}

// PFS_DEBBY__EXPORT optional<result::value_type> result::get_impl (std::string const & name)
// {
//     auto colimn = column_name_impl
// }

}}} // namespace pfs::debby::sqlite3


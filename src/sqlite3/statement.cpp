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
#include "pfs/bits/compiler.h"
#include "pfs/fmt.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include <cstring>
#include <cassert>

namespace pfs {
namespace debby {
namespace sqlite3 {

std::string const statement::ERROR_DOMAIN {"SQLITE3"};

std::string statement::current_sql () const noexcept
{
    assert(_sth);
    return sqlite3::current_sql(_sth);
}

void statement::clear_impl () noexcept
{
    if (_sth) {
        if (!_cached)
            sqlite3_finalize(_sth);
        else
            sqlite3_reset(_sth);
    }

    _sth = nullptr;
}

statement::result_type statement::exec_impl ()
{
    assert(_sth);

    result_type::status status {result_type::INITIAL};
    int rc = sqlite3_step(_sth);

    switch (rc) {
        case SQLITE_ROW: {
            status = result_type::ROW;
            break;
        }

        case SQLITE_DONE: {
            status = result_type::DONE;
            break;
        }

        case SQLITE_CONSTRAINT:
        case SQLITE_ERROR: {
            PFS_DEBBY_THROW((sql_error{
                  ERROR_DOMAIN
                , fmt::format("statement execution failure: {}", build_errstr(rc, _sth))
                , current_sql()
            }));

            status = result_type::ERROR;
            break;
        }

        case SQLITE_MISUSE:
        case SQLITE_BUSY:
        default: {
            PFS_DEBBY_THROW((sql_error{
                  ERROR_DOMAIN
                , fmt::format("statement execution failure: {}", build_errstr(rc, _sth))
                , current_sql()
            }));

            status = result_type::ERROR;
            break;
        }
    }

    if (rc != SQLITE_ROW)
        sqlite3_reset(_sth);

    return result_type{_sth, status};
}

bool statement::bind_helper (std::string const & placeholder
    , std::function<int (int /*index*/)> && bind_wrapper)
{
    assert(_sth);

    bool success = true;
    int index = sqlite3_bind_parameter_index(_sth, placeholder.c_str());

    if (index == 0) {
        PFS_DEBBY_THROW((sql_error{
                ERROR_DOMAIN
            , fmt::format("bad bind parameter name: {}", placeholder)
            , current_sql()
        }));

        success = false;
    }

    if (success) {
        int rc = bind_wrapper(index);

        if (SQLITE_OK != rc) {
            PFS_DEBBY_THROW((sql_error{
                  ERROR_DOMAIN
                , fmt::format("bind failure: {}", build_errstr(rc, _sth))
                , current_sql()
            }));

            success = false;
        }
    }

    return success;
}

bool statement::bind_impl (std::string const & placeholder, std::nullptr_t)
{
    return bind_helper(placeholder, [this] (int index) {
        return sqlite3_bind_null(_sth, index);
    });
}

bool statement::bind_impl (std::string const & placeholder, bool value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, (value ? 1 : 0));
    });
}

bool statement::bind_impl (std::string const & placeholder, std::int8_t value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, std::uint8_t value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, std::int16_t value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, std::uint16_t value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, std::int32_t value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, std::uint32_t value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int(_sth, index, static_cast<int>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, std::int64_t value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int64(_sth, index, static_cast<sqlite3_int64>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, std::uint64_t value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_int64(_sth, index, static_cast<sqlite3_int64>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, float value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_double(_sth, index, static_cast<double>(value));
    });
}

bool statement::bind_impl (std::string const & placeholder, double value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_double(_sth, index, value);
    });
}

bool statement::bind_impl (std::string const & placeholder, char const * value, std::size_t len)
{
    if (value == nullptr) {
        return bind_impl(placeholder, nullptr);
    } else {
        return bind_helper(placeholder, [this, value, len] (int index) {
            return sqlite3_bind_text(_sth, index, value, static_cast<int>(len), SQLITE_TRANSIENT);
        });
    }
}

bool statement::bind_impl (std::string const & placeholder, std::vector<std::uint8_t> const & value)
{
    if (value.size() > std::numeric_limits<int>::max()) {
        auto data = value.data();
        sqlite3_uint64 len = value.size();

        return bind_helper(placeholder, [this, data, len] (int index) {
            return sqlite3_bind_blob64(_sth, index, data, len, SQLITE_TRANSIENT);
        });
    } else {
        auto data = value.data();
        int len = static_cast<int>(value.size());

        return bind_helper(placeholder, [this, data, len] (int index) {
            return sqlite3_bind_blob(_sth, index, data, len, SQLITE_TRANSIENT);
        });
    }
}

}}} // namespace pfs::debby::sqlite3

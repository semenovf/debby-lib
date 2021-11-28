////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/bits/compiler.h"
#include "pfs/fmt.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include <cstring>
#include <cassert>

namespace pfs {
namespace debby {
namespace sqlite3 {

void statement::clear_impl ()
{
    if (_sth) {
        sqlite3_finalize(_sth);
        _sth = nullptr;
    }
}

result statement::exec_impl ()
{
    if (!_sth) {
        _last_error = "statement not initialized";
        return result{nullptr, result::ERROR};
    }

    // Bindings are not cleared by the sqlite3_reset() routine
    int rc = sqlite3_reset(_sth);

    if (SQLITE_OK != rc) {
        _last_error = sqlite3_errstr(rc);
        clear_impl();
        return result{nullptr, result::ERROR};
    }

    rc = sqlite3_step(_sth);
    result ret;

    switch (rc) {
        case SQLITE_ROW: {
            ret = result{_sth, result::ROW};
            break;
        }

        case SQLITE_DONE: {
            ret = result{_sth, result::DONE};
            break;
        }

        case SQLITE_CONSTRAINT:
        case SQLITE_ERROR: {
            ret = result{_sth, result::ERROR};
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
        default: {
            ret = result{_sth, result::ERROR};
            _last_error = sqlite3_errstr(rc);
            break;
        }
    }

    return ret;
}

bool statement::bind_helper (std::string const & placeholder
    , std::function<int (int /*index*/)> && bind_wrapper)
{
    if (!_sth) {
        _last_error = "statement not initialized";
        return false;
    }

    bool success = true;
    int index = sqlite3_bind_parameter_index(_sth, placeholder.c_str());

    if (index == 0) {
        _last_error = fmt::format("bad bind parameter name: {}", placeholder);
        success = false;
    }

    if (success) {
        int rc = bind_wrapper(index);

        if (SQLITE_OK != rc) {
            _last_error = sqlite3_errstr(rc);
            return false;
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

bool statement::bind_impl (std::string const & placeholder, std::string const & value)
{
    char const * text = value.c_str();
    int len = static_cast<int>(value.size());

    return bind_helper(placeholder, [this, text, len] (int index) {
        return sqlite3_bind_text(_sth, index, text, len, SQLITE_TRANSIENT);
    });
}

bool statement::bind_impl (std::string const & placeholder, char const * value)
{
    return bind_helper(placeholder, [this, value] (int index) {
        return sqlite3_bind_text(_sth, index, value, std::strlen(value), SQLITE_STATIC);
    });
}

}}} // namespace pfs::debby::sqlite3

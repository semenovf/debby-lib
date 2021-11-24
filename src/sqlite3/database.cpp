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
#include "pfs/debby/sqlite3/database.hpp"
#include <cassert>

namespace pfs {
namespace debby {
namespace sqlite3 {

namespace {
int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second
}

bool database::query (std::string const & sql)
{
    assert(_dbh);

    char * errmsg {nullptr};
    int rc = sqlite3_exec(_dbh, sql.c_str(), nullptr, nullptr, & errmsg);

    if (SQLITE_OK != rc) {
        if (errmsg)
            _last_error = errmsg;
        else
            _last_error = "sqlite3_exec() failure";

        return false;
    }

    return true;
}

bool database::open_impl (filesystem::path const & path)
{
    if (_dbh) {
        _last_error = "database already opened";
        return false;
    }

    int flags = 0; //SQLITE_OPEN_URI;

    // It is an error to specify a value for the mode parameter
    // that is less restrictive than that specified by the flags passed
    // in the third parameter to sqlite3_open_v2().
    flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

    // Use default `sqlite3_vfs` object.
    char const * default_vfs = nullptr;

#if PFS_COMPILER_MSVC
    auto utf8_path = pfs::utf8_encode(path.c_str(), std::wcslen(path.c_str()));
    int rc = sqlite3_open_v2(utf8_path.c_str(), & _dbh, flags, default_vfs);
#else
    int rc = sqlite3_open_v2(path.c_str(), & _dbh, flags, default_vfs);
#endif

    bool success = true;

    if (rc != SQLITE_OK) {
        if (!_dbh) {
            // Unable to allocate memory for database handler.
            // Internal error code.
            _last_error = "unable to allocate memory for database handler";
        } else {
            _last_error = fmt::format("sqlite3_open_v2(): {}", sqlite3_errstr(rc));
            sqlite3_close_v2(_dbh);
            _dbh = nullptr;
        }

        success = false;
    } else {
        // TODO what for this call ?
        sqlite3_busy_timeout(_dbh, MAX_BUSY_TIMEOUT);

        // Enable extended result codes
        sqlite3_extended_result_codes(_dbh, 1);

        success = query("PRAGMA foreign_keys = ON");

        if (!success) {
            sqlite3_close_v2(_dbh);
            _dbh = nullptr;
        }
    }

    return success;
}

void database::close_impl ()
{
    if (_dbh)
        sqlite3_close_v2(_dbh);
    _dbh = nullptr;
};

statement database::prepare_impl (std::string const & sql)
{
    if (!_dbh) {
        _last_error = "database not open";
        return statement{};
    }

    sqlite3_stmt * sth {nullptr};

    auto rc = sqlite3_prepare_v2(_dbh, sql.c_str(), sql.size(), & sth, nullptr);

    if (SQLITE_OK != rc) {
        _last_error = sqlite3_errmsg(_dbh);
        assert(sth == nullptr);
        return statement{};
    }

    return statement{sth};
}

}}} // namespace pfs::debby::sqlite3

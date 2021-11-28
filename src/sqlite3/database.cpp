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
#include <regex>
#include <cassert>

namespace pfs {
namespace debby {
namespace sqlite3 {

namespace {
int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second

std::string ALREADY_OPEN_ERROR  {"database is already open"};
std::string NOT_OPEN_ERROR      {"database not open"};
std::string ALLOC_MEMORY_ERROR  {"unable to allocate memory for database handler"};
} // namespace

bool database::query_impl (std::string const & sql)
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
        _last_error = ALREADY_OPEN_ERROR;
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
            _last_error = ALLOC_MEMORY_ERROR;
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

bool database::clear_impl ()
{
    if (!_dbh) {
        _last_error = NOT_OPEN_ERROR;
        return false;
    }

    auto list = tables_impl(std::string{});
    auto success = query("PRAGMA foreign_keys = OFF");

    if (!success)
        return false;

    for (auto const & t: list) {
        auto sql = fmt::format("DROP TABLE IF EXISTS `{}`", t);
        success = query(sql);

        if (!success)
            break;
    }

    success = query("PRAGMA foreign_keys = ON") && success;

    return success;
}

std::vector<std::string> database::tables_impl (std::string const & pattern)
{
    statement_type stmt = prepare(
        "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");

    std::vector<std::string> list;
    std::regex rx {pattern.empty() ? ".*" : pattern};

    if (stmt) {
        auto res = stmt.exec();

        while (res.has_more()) {
            auto value = res.get(0);

            auto success = value.has_value() && holds_alternative<std::string>(*value);
            assert(success);

            if (pattern.empty()) {
                list.push_back(get<std::string>(*value));
            } else {
                if (std::regex_search(get<std::string>(*value), rx))
                    list.push_back(get<std::string>(*value));
            }

            res.next();
        }

        if (res.is_error()) {
            _last_error = res.last_error();
            return std::vector<std::string>{};
        } else {
            assert(res.is_done());
        }
    }

    return list;
}

bool database::exists_impl (std::string const & name)
{
    statement_type stmt = prepare(
        fmt::format("SELECT name FROM sqlite_master"
            " WHERE type='table' AND name='{}'"
            , name));

    if (stmt) {
        auto res = stmt.exec();

        if (res.has_more())
            return true;
    }

    return false;
}

statement database::prepare_impl (std::string const & sql)
{
    if (!_dbh) {
        _last_error = NOT_OPEN_ERROR;
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

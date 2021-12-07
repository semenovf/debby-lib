////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3.h"
#include "errstr_builder.hpp"
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

PFS_DEBBY__EXPORT bool database::query_impl (std::string const & sql)
{
    assert(_dbh);

    int rc = sqlite3_exec(_dbh, sql.c_str(), nullptr, nullptr, nullptr);
    bool success = SQLITE_OK == rc;

    if (! success) {
        _last_error = build_errstr("query failure", rc, _dbh);
    }

    return success;
}

PFS_DEBBY__EXPORT bool database::open_impl (filesystem::path const & path)
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
        // | SQLITE_OPEN_PRIVATECACHE;

    // Use default `sqlite3_vfs` object.
    char const * default_vfs = nullptr;

    assert(sqlite3_enable_shared_cache(0) == SQLITE_OK);

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
            _last_error = build_errstr("sqlite3_open_v2", rc, _dbh);
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

PFS_DEBBY__EXPORT void database::close_impl ()
{
    // Finalize cached statements
    for (auto & item: _cache)
        sqlite3_finalize(item.second);

    _cache.clear();

    if (_dbh)
        sqlite3_close_v2(_dbh);

    _dbh = nullptr;
};

PFS_DEBBY__EXPORT bool database::clear_impl ()
{
    if (!_dbh) {
        _last_error = NOT_OPEN_ERROR;
        return false;
    }

    auto list = tables_impl(std::string{});

    auto success = begin_impl() && query_impl("PRAGMA foreign_keys = OFF");

    if (!success)
        return false;

    for (auto const & t: list) {
        auto sql = fmt::format("DROP TABLE IF EXISTS `{}`", t);
        success = query_impl(sql);

        if (!success)
            break;
    }

    success = success && query_impl("PRAGMA foreign_keys = ON");

    if (success) {
        commit_impl();
    } else {
        rollback_impl();
    }

    return success;
}

PFS_DEBBY__EXPORT std::vector<std::string> database::tables_impl (std::string const & pattern)
{
    statement_type stmt = prepare(
        "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");

    std::vector<std::string> list;
    std::regex rx {pattern.empty() ? ".*" : pattern};

    if (stmt) {
        auto res = stmt.exec();

        while (res.has_more()) {
            auto value = res.fetch(0);

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

PFS_DEBBY__EXPORT bool database::exists_impl (std::string const & name)
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

PFS_DEBBY__EXPORT statement database::prepare_impl (std::string const & sql, bool cache)
{
    if (!_dbh) {
        _last_error = NOT_OPEN_ERROR;
        return statement{};
    }

    auto pos = _cache.find(sql);

    // Found in cache
    if (pos != _cache.end()) {
        sqlite3_reset(pos->second);
        sqlite3_clear_bindings(pos->second);
        return statement{pos->second, true};
    }

    statement_type::native_type sth {nullptr};

    auto rc = sqlite3_prepare_v2(_dbh, sql.c_str(), sql.size(), & sth, nullptr);

    if (SQLITE_OK != rc) {
        _last_error = sqlite3_errmsg(_dbh);
        assert(sth == nullptr);
        return statement{};
    }

    if (cache) {
        auto res = _cache.emplace(sql, sth);
        assert(res.second); // key must be unique
    }

    return statement{sth, cache};
}

PFS_DEBBY__EXPORT bool database::begin_impl ()
{
    return query_impl("BEGIN TRANSACTION");
}

PFS_DEBBY__EXPORT bool database::commit_impl ()
{
    return query_impl("COMMIT TRANSACTION");
}

PFS_DEBBY__EXPORT bool database::rollback_impl ()
{
    return query_impl("ROLLBACK TRANSACTION");
}

}}} // namespace pfs::debby::sqlite3

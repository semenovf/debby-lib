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
#include "pfs/debby/sqlite3/database.hpp"
#include <regex>
#include <cassert>

namespace pfs {
namespace debby {
namespace sqlite3 {

std::string const database::ERROR_DOMAIN {"SQLITE3"};

namespace {

int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second

} // namespace

bool database::open_impl (filesystem::path const & path, bool create_if_missing)
{
    if (_dbh) {
        PFS_DEBBY_THROW((runtime_error{
              ERROR_DOMAIN
            , fmt::format("database is already open: {}", path.c_str())
        }));

        return false;
    }

    int flags = 0; //SQLITE_OPEN_URI;

    // It is an error to specify a value for the mode parameter
    // that is less restrictive than that specified by the flags passed
    // in the third parameter to sqlite3_open_v2().
    flags |= SQLITE_OPEN_READWRITE;
        // | SQLITE_OPEN_PRIVATECACHE;

    if (create_if_missing)
        flags |= SQLITE_OPEN_CREATE;

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
            PFS_DEBBY_THROW((bad_alloc{
                  ERROR_DOMAIN
                , fmt::format("unable to allocate memory for database handler: {}", path.c_str())
            }));
        } else {
            PFS_DEBBY_THROW((runtime_error{
                  ERROR_DOMAIN
                , fmt::format("database open failure: {}", path.c_str())
                , build_errstr(rc, _dbh)
            }));

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
    // Finalize cached statements
    for (auto & item: _cache)
        sqlite3_finalize(item.second);

    _cache.clear();

    if (_dbh)
        sqlite3_close_v2(_dbh);

    _dbh = nullptr;
};

database::statement_type database::prepare_impl (std::string const & sql, bool cache)
{
    assert(_dbh);

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
        PFS_DEBBY_THROW((sql_error{
              ERROR_DOMAIN
            , fmt::format("prepare statement failure: {}", build_errstr(rc, _dbh))
            , sql
        }));

        return statement{sth, false};
    }

    if (cache) {
        auto res = _cache.emplace(sql, sth);
        assert(res.second); // key must be unique
    }

    return statement{sth, cache};
}

bool database::query_impl (std::string const & sql)
{
    assert(_dbh);

    int rc = sqlite3_exec(_dbh, sql.c_str(), nullptr, nullptr, nullptr);
    bool success = SQLITE_OK == rc;

    if (! success) {
        PFS_DEBBY_THROW((sql_error{
              ERROR_DOMAIN
            , fmt::format("query failure: {}", build_errstr(rc, _dbh))
            , sql
        }));
    }

    return success;
}

std::vector<std::string> database::tables_impl (std::string const & pattern)
{
    std::string sql = std::string{"SELECT name FROM sqlite_master "
        "WHERE type='table' ORDER BY name"};

    statement_type stmt = prepare(sql);

    std::vector<std::string> list;
    std::regex rx {pattern.empty() ? ".*" : pattern};

    if (stmt) {
        // may be `sql_error` exception
        auto res = stmt.exec();

        while (res.has_more()) {
            decltype(res.fetch(0)) value;

            try {
                // may be `invalid_argument` but it is unexpected.
                value = res.fetch(0);
            } catch (invalid_argument ex) {
                assert(false);
            }

            auto success = holds_alternative<std::string>(value);

            // Unexpected
            assert(success);

            if (pattern.empty()) {
                list.push_back(get<std::string>(value));
            } else {
                if (std::regex_search(get<std::string>(value), rx))
                    list.push_back(get<std::string>(value));
            }

            // may be `sql_error` exception
            res.next();
        }

        // Expecting done
        assert(res.is_done());
    }

    return list;
}

bool database::clear_impl ()
{
    assert(_dbh);

    auto list = tables_impl(std::string{});
    bool success = begin_impl();

    if (success) {
        do {
            success = query_impl("PRAGMA foreign_keys = OFF");

            if (!success)
                break;

            for (auto const & t: list) {
                auto sql = fmt::format("DROP TABLE IF EXISTS `{}`", t);
                success = query_impl(sql);

                if (!success)
                    break;
            }

            if (!success)
                break;

            success = query_impl("PRAGMA foreign_keys = ON");

            if (!success)
                break;
        } while (false);

        if (success)
            commit_impl();
        else
            rollback_impl();
    }

    return success;
}

bool database::exists_impl (std::string const & name)
{
    auto stmt = prepare(
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

bool database::begin_impl ()
{
    return query_impl("BEGIN TRANSACTION");
}

bool database::commit_impl ()
{
    return query_impl("COMMIT TRANSACTION");
}

bool database::rollback_impl ()
{
    return query_impl("ROLLBACK TRANSACTION");
}

}}} // namespace pfs::debby::sqlite3

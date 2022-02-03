////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3.h"
#include "utils.hpp"
#include "pfs/bits/compiler.h"
#include "pfs/fmt.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/sqlite3/database.hpp"
#include <regex>

namespace debby {
namespace sqlite3 {

namespace fs = pfs::filesystem;

namespace {
int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second
char const * NULL_HANDLER = "uninitialized database handler";
} // namespace

bool database::open (pfs::filesystem::path const & path
    , bool create_if_missing
    , error * perr) noexcept
{
    DEBBY__ASSERT(!_dbh, NULL_HANDLER);

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

    std::error_code ec;

    if (rc != SQLITE_OK) {
        if (!_dbh) {
            // Unable to allocate memory for database handler.
            // Internal error code.
            auto err = error{make_error_code(errc::bad_alloc)};
            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        } else {
            sqlite3_close_v2(_dbh);
            _dbh = nullptr;

            bool is_special_file = path.empty()
                || *path.c_str() == PFS__LITERAL_PATH(':');

            if (rc == SQLITE_CANTOPEN && !is_special_file) {
                ec = make_error_code(errc::database_not_found);
            } else {
                ec = make_error_code(errc::backend_error);
            }

            auto err = error{ec
                , fs::utf8_encode(path)
                , build_errstr(rc, _dbh)};

            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        }
    } else {
        // TODO what for this call ?
        sqlite3_busy_timeout(_dbh, MAX_BUSY_TIMEOUT);

        // Enable extended result codes
        sqlite3_extended_result_codes(_dbh, 1);

        auto success = query_impl("PRAGMA foreign_keys = ON", perr);

        if (!success) {
            sqlite3_close_v2(_dbh);
            _dbh = nullptr;
            return false;
        }
    }

    return true;
}

void database::close () noexcept
{
    // Finalize cached statements
    for (auto & item: _cache)
        sqlite3_finalize(item.second);

    _cache.clear();

    if (_dbh)
        sqlite3_close_v2(_dbh);

    _dbh = nullptr;
};

std::size_t database::rows_count_impl (std::string const & table_name)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    std::size_t count = 0;
    std::string sql = fmt::format("SELECT COUNT(1) as count FROM `{}`"
        , table_name);

    error err;
    statement_type stmt = prepare_impl(sql, true, & err);

    if (stmt) {
        auto res = stmt.exec(& err);

        if (res) {
            if (res.has_more()) {
                auto value = res.fetch(0, & err);

                // Unexpected result
                DEBBY__ASSERT(pfs::holds_alternative<std::intmax_t>(value), "");
                count = static_cast<std::size_t>(pfs::get<std::intmax_t>(value));

                // may be `sql_error` exception
                res.next();
            }
        }

        // Expecting done
        DEBBY__ASSERT(res.is_done(), "");
    }

    return count;
}

database::statement_type database::prepare_impl (std::string const & sql
    , bool cache
    , error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    auto pos = _cache.find(sql);

    // Found in cache
    if (pos != _cache.end()) {
        sqlite3_reset(pos->second);
        sqlite3_clear_bindings(pos->second);
        return statement_type{pos->second, true};
    }

    statement_type::native_type sth {nullptr};

    auto rc = sqlite3_prepare_v2(_dbh, sql.c_str(), sql.size(), & sth, nullptr);

    if (SQLITE_OK != rc) {
        assert(!sth);
        auto ec = make_error_code(errc::sql_error);
        auto err = error{ec, build_errstr(rc, _dbh), sql};
        if (perr) *perr = err; else DEBBY__THROW(err);
        return statement{nullptr, false};
    }

    if (cache) {
        auto res = _cache.emplace(sql, sth);
        assert(res.second); // key must be unique
    }

    return statement{sth, cache};
}

bool database::query_impl (std::string const & sql, error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    int rc = sqlite3_exec(_dbh, sql.c_str(), nullptr, nullptr, nullptr);

    if (SQLITE_OK != rc) {
        auto ec = make_error_code(errc::sql_error);
        auto err = error{ec, build_errstr(rc, _dbh), sql};
        if (perr) *perr = err; else DEBBY__THROW(err);
        return false;
    }

    return true;
}

std::vector<std::string> database::tables_impl (std::string const & pattern
    , error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    std::string sql = std::string{"SELECT name FROM sqlite_master "
        "WHERE type='table' ORDER BY name"};

    statement_type stmt = prepare_impl(sql, true, perr);

    std::vector<std::string> list;

    if (stmt) {
        auto res = stmt.exec(perr);

        if (res) {
            std::regex rx {pattern.empty() ? ".*" : pattern};

            while (res.has_more()) {
                auto value = res.fetch(0, perr);

                // Unexpected result
                DEBBY__ASSERT(pfs::holds_alternative<std::string>(value), "");

                if (pattern.empty()) {
                    list.push_back(pfs::get<std::string>(value));
                } else {
                    if (std::regex_search(pfs::get<std::string>(value), rx))
                        list.push_back(pfs::get<std::string>(value));
                }

                // may be `sql_error` exception
                res.next();
            }
        }

        // Expecting done
        DEBBY__ASSERT(res.is_done(), "");
    }

    return list;
}

auto database::clear_impl (std::string const & table, error * perr) -> bool
{
    return query_impl(fmt::format("DELETE FROM `{}`", table), perr);
}

auto database::remove_impl (error * perr) -> bool
{
    error err;
    auto tables = tables_impl(std::string{}, & err);

    if (tables.empty()) {
        if (err && perr)
            *perr = err;

        return !err;
    }

    return remove_impl(tables.cbegin(), tables.cend(), perr);
}

bool database::remove_impl (std::vector<std::string>::const_iterator first
        , std::vector<std::string>::const_iterator last, error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    if (first == last)
        return true;

    bool success = begin_impl();

    if (success) {
        do {
            success = !!query_impl("PRAGMA foreign_keys = OFF", perr);

            if (!success)
                break;

            for (; first != last; ++first) {
                auto sql = fmt::format("DROP TABLE IF EXISTS `{}`", *first);
                success = !!query_impl(sql, perr);

                if (!success)
                    break;
            }

            if (!success)
                break;

            success = !!query_impl("PRAGMA foreign_keys = ON", perr);

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

bool database::exists_impl (std::string const & name, error * perr)
{
    auto stmt = prepare(
        fmt::format("SELECT name FROM sqlite_master"
            " WHERE type='table' AND name='{}'"
            , name));

    if (stmt) {
        auto res = stmt.exec(perr);

        if (res.has_more())
            return true;
    }

    return false;
}

bool database::begin_impl ()
{
    return query_impl("BEGIN TRANSACTION", nullptr);
}

bool database::commit_impl ()
{
    return query_impl("COMMIT TRANSACTION", nullptr);
}

bool database::rollback_impl ()
{
    return query_impl("ROLLBACK TRANSACTION", nullptr);
}

}} // namespace debby::sqlite3

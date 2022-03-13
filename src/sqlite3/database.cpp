////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/fmt.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/relational_database.hpp"
#include "pfs/debby/backend/sqlite3/database.hpp"
#include "sqlite3.h"
#include "utils.hpp"
#include <regex>

namespace fs = pfs::filesystem;

namespace debby {

int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second
char const * NULL_HANDLER = "uninitialized database handler";

namespace backend {
namespace sqlite3 {

static result_status query (database::rep_type const * rep, std::string const & sql)
{
    DEBBY__ASSERT(rep->dbh, NULL_HANDLER);

    int rc = sqlite3_exec(rep->dbh, sql.c_str(), nullptr, nullptr, nullptr);

    if (SQLITE_OK != rc) {
        auto err = error{make_error_code(errc::sql_error)
            , build_errstr(rc, rep->dbh)
            , sql
        };

        return err;
    }

    return result_status{};
}

database::rep_type
database::make (fs::path const & path, bool create_if_missing)
{
    rep_type rep;
    rep.dbh = nullptr;

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

    PFS__ASSERT(sqlite3_enable_shared_cache(0) == SQLITE_OK, "");

#if PFS_COMPILER_MSVC
    auto utf8_path = pfs::utf8_encode(path.c_str(), std::wcslen(path.c_str()));
    int rc = sqlite3_open_v2(utf8_path.c_str(), & rep.dbh, flags, default_vfs);
#else
    int rc = sqlite3_open_v2(path.c_str(), & rep.dbh, flags, default_vfs);
#endif

    std::error_code ec;

    if (rc != SQLITE_OK) {
        if (!rep.dbh) {
            // Unable to allocate memory for database handler.
            // Internal error code.
            auto err = error{
                make_error_code(errc::bad_alloc)
            };

            DEBBY__THROW(err);
        } else {
            sqlite3_close_v2(rep.dbh);
            rep.dbh = nullptr;

            bool is_special_file = path.empty()
                || *path.c_str() == PFS__LITERAL_PATH(':');

            if (rc == SQLITE_CANTOPEN && !is_special_file) {
                ec = make_error_code(errc::database_not_found);
            } else {
                ec = make_error_code(errc::backend_error);
            }

            auto err = error{
                  ec
                , fs::utf8_encode(path)
                , build_errstr(rc, rep.dbh)
            };

            DEBBY__THROW(err);
        }
    } else {
        // NOTE what for this call ?
        sqlite3_busy_timeout(rep.dbh, MAX_BUSY_TIMEOUT);

        // Enable extended result codes
        sqlite3_extended_result_codes(rep.dbh, 1);

        auto res = query(& rep, "PRAGMA foreign_keys = ON");

        if (!res.ok()) {
            sqlite3_close_v2(rep.dbh);
            rep.dbh = nullptr;
            DEBBY__THROW(res);
        }
    }

    return rep;
}

}} // namespace backend::sqlite3

#define BACKEND backend::sqlite3::database

template <>
relational_database<BACKEND>::relational_database (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
relational_database<BACKEND>::relational_database (relational_database && other)
{
    _rep = std::move(other._rep);
    other._rep.dbh = nullptr;
}

template <>
relational_database<BACKEND>::~relational_database ()
{
    // Finalize cached statements
    for (auto & item: _rep.cache)
        sqlite3_finalize(item.second);

    _rep.cache.clear();

    if (_rep.dbh)
        sqlite3_close_v2(_rep.dbh);

    _rep.dbh = nullptr;
}

template <>
relational_database<BACKEND>::operator bool () const noexcept
{
    return _rep.dbh != nullptr;
}

template <>
void
relational_database<BACKEND>::query (std::string const & sql)
{
    auto res = backend::sqlite3::query(& _rep, sql);

    if (!res.ok())
        DEBBY__THROW(res);
}

template <>
void
relational_database<BACKEND>::begin ()
{
    query("BEGIN TRANSACTION");
}

template <>
void
relational_database<BACKEND>::commit ()
{
    query("COMMIT TRANSACTION");
}

template <>
void
relational_database<BACKEND>::rollback ()
{
    query("ROLLBACK TRANSACTION");
}

template <>
relational_database<BACKEND>::statement_type
relational_database<BACKEND>::prepare (std::string const & sql, bool cache)
{
    DEBBY__ASSERT(_rep.dbh, NULL_HANDLER);

    auto pos = _rep.cache.find(sql);

    // Found in cache
    if (pos != _rep.cache.end()) {
        sqlite3_reset(pos->second);
        sqlite3_clear_bindings(pos->second);
        return statement_type::make(pos->second, true);
    }

    struct sqlite3_stmt * sth {nullptr};

    auto rc = sqlite3_prepare_v2(_rep.dbh, sql.c_str(), sql.size(), & sth, nullptr);

    if (SQLITE_OK != rc) {
        error err {
              make_error_code(errc::sql_error)
            , backend::sqlite3::build_errstr(rc, _rep.dbh)
            , sql
        };

        DEBBY__THROW(err);
    }

    if (cache) {
        auto res = _rep.cache.emplace(sql, sth);
        DEBBY__ASSERT(res.second, "key must be unique");
    }

    return statement_type::make(sth, cache);
}

template <>
std::size_t
relational_database<BACKEND>::rows_count (std::string const & table_name)
{
    DEBBY__ASSERT(_rep.dbh, NULL_HANDLER);

    std::size_t count = 0;
    std::string sql = fmt::format("SELECT COUNT(1) as count FROM `{}`"
        , table_name);

    statement_type stmt = prepare(sql);

    if (stmt) {
        auto res = stmt.exec();

        if (res) {
            if (res.has_more()) {
                count = res.get<std::size_t>(0);

                // May be `sql_error` exception
                res.next();
            }
        }

        // Expecting done
        DEBBY__ASSERT(res.is_done(), "expecting done");
    }

    return count;
}

template <>
std::vector<std::string>
relational_database<BACKEND>::tables (std::string const & pattern)
{
    DEBBY__ASSERT(_rep.dbh, NULL_HANDLER);

    std::string sql = std::string{"SELECT name FROM sqlite_master "
        "WHERE type='table' ORDER BY name"};

    auto stmt = prepare(sql);
    std::vector<std::string> list;

    if (stmt) {
        auto res = stmt.exec();

        if (res) {
            std::regex rx {pattern.empty() ? ".*" : pattern};

            while (res.has_more()) {
                auto name = res.get<std::string>(0);

                if (pattern.empty()) {
                    list.push_back(name);
                } else {
                    if (std::regex_search(name, rx))
                        list.push_back(name);
                }

                // May be `sql_error` exception
                res.next();
            }
        }

        DEBBY__ASSERT(res.is_done(), "expecting done");
    }

    return list;
}
template <>
void
relational_database<BACKEND>::clear (std::string const & table)
{
    query(fmt::format("DELETE FROM `{}`", table));
}

template <>
void
relational_database<BACKEND>::remove (std::vector<std::string> const & tables)
{
    DEBBY__ASSERT(_rep.dbh, NULL_HANDLER);

    if (tables.empty())
        return;

    begin();

    try {
        query("PRAGMA foreign_keys = OFF");

        for (auto const & name: tables) {
            auto sql = fmt::format("DROP TABLE IF EXISTS `{}`", name);
            query(sql);
        }

        query("PRAGMA foreign_keys = ON");
        commit();
    } catch (error ex) {
        rollback();
#if PFS__EXCEPTIONS_ENABLED
        throw;
#endif
    }
}

template <>
void
relational_database<BACKEND>::remove_all ()
{
    auto list = tables(std::string{});
    remove(list);
}

template <>
bool
relational_database<BACKEND>::exists (std::string const & name)
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

} // namespace debby

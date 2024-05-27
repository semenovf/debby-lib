////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2023.02.07 Applied new API.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/debby/error.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/relational_database.hpp"
#include "pfs/debby/backend/sqlite3/database.hpp"
#include "../kv_common.hpp"
#include "sqlite3.h"
#include "utils.hpp"
#include <pfs/bits/compiler.h>
#include <pfs/filesystem.hpp>
#include <pfs/i18n.hpp>
#include <regex>

namespace fs = pfs::filesystem;

namespace debby {

int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second
char const * NULL_HANDLER = tr::noop_("uninitialized database handler");

namespace backend {
namespace sqlite3 {

static bool query (database::rep_type const * rep, std::string const & sql, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    int rc = sqlite3_exec(rep->dbh, sql.c_str(), nullptr, nullptr, nullptr);

    if (SQLITE_OK != rc) {
        error err {
              errc::sql_error
            , build_errstr(rc, rep->dbh)
            , sql
        };

        if (perr) {
            *perr = std::move(err);
            return false;
        } else {
            throw err;
        }
    }

    return true;
}

database::rep_type
database::make_r (fs::path const & path, bool create_if_missing, error * perr)
{
    return make_r(path, create_if_missing, make_options{}, perr);
}

database::rep_type
database::make_r (pfs::filesystem::path const & path, bool create_if_missing
    , presets_enum preset, error * perr)
{
    switch (preset) {
        // See https://phiresky.github.io/blog/2020/sqlite-performance-tuning/
        case database::CONCURRENCY_PRESET: {
            make_options opts;
            opts.pragma_journal_mode = JM_WAL;
            opts.pragma_synchronous = SYN_NORMAL;
            opts.pragma_temp_store = TS_MEMORY;
            opts.pragma_mmap_size = 30000000000UL;

            return make_r(path, create_if_missing, std::move(opts), perr);
        }

        case database::DEFAULT_PRESET:
        default:
            break;
    }

    return make_r(path, create_if_missing, make_options{}, perr);
}

database::rep_type
database::make_r (pfs::filesystem::path const & path, bool create_if_missing
    , make_options && opts, error * perr)
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

#if PFS__COMPILER_MSVC
    auto utf8_path = pfs::filesystem::utf8_encode(path);
    int rc = sqlite3_open_v2(utf8_path.c_str(), & rep.dbh, flags, default_vfs);
#else
    int rc = sqlite3_open_v2(path.c_str(), & rep.dbh, flags, default_vfs);
#endif

    if (rc != SQLITE_OK) {
        if (!rep.dbh) {
            // Unable to allocate memory for database handler.
            // Internal error code.
            throw error { errc::bad_alloc };
        } else {
            sqlite3_close_v2(rep.dbh);
            rep.dbh = nullptr;

            bool is_special_file = path.empty()
                || *path.c_str() == PFS__LITERAL_PATH(':');

            errc e = errc::success;

            if (rc == SQLITE_CANTOPEN && !is_special_file) {
                e = errc::database_not_found;
            } else {
                e = errc::backend_error;
            }

            pfs::throw_or(perr, error { e, fs::utf8_encode(path), build_errstr(rc, rep.dbh) });
            return rep_type{};
        }
    } else {
        // NOTE what for this call ?
        sqlite3_busy_timeout(rep.dbh, MAX_BUSY_TIMEOUT);

        // Enable extended result codes
        sqlite3_extended_result_codes(rep.dbh, 1);

        std::vector<std::string> pragmas;

        if (opts.pragma_journal_mode) {
            switch (*opts.pragma_journal_mode) {
                case JM_DELETE:
                    pragmas.emplace_back("pragma journal_mode = DELETE");
                    break;
                case JM_TRUNCATE:
                    pragmas.emplace_back("pragma journal_mode = TRUNCATE");
                    break;
                case JM_PERSIST:
                    pragmas.emplace_back("pragma journal_mode = PERSIST");
                    break;
                case JM_MEMORY:
                    pragmas.emplace_back("pragma journal_mode = MEMORY");
                    break;
                case JM_WAL:
                    pragmas.emplace_back("pragma journal_mode = WAL");
                    break;
                case JM_OFF:
                    pragmas.emplace_back("pragma journal_mode = OFF");
                    break;
            }
        }

        if (opts.pragma_synchronous) {
            switch (*opts.pragma_synchronous) {
                case SYN_OFF:
                    pragmas.emplace_back("pragma synchronous = OFF");
                    break;
                case SYN_NORMAL:
                    pragmas.emplace_back("pragma synchronous = NORMAL");
                    break;
                case SYN_FULL:
                    pragmas.emplace_back("pragma synchronous = FULL");
                    break;
                case SYN_EXTRA:
                    pragmas.emplace_back("pragma synchronous = OFF");
                    break;
            }
        }


        if (opts.pragma_temp_store) {
            switch (*opts.pragma_temp_store) {
                case TS_DEFAULT:
                    pragmas.emplace_back("pragma temp_store = DEFAULT");
                    break;
                case TS_FILE:
                    pragmas.emplace_back("pragma temp_store = FILE");
                    break;
                case TS_MEMORY:
                    pragmas.emplace_back("pragma temp_store = MEMORY");
                    break;
            }
        }

        if (opts.pragma_mmap_size)
            pragmas.emplace_back(fmt::format("pragma mmap_size = {}", *opts.pragma_mmap_size));

        pragmas.emplace_back("PRAGMA foreign_keys = ON");

        for (auto const & pragma: pragmas) {
            error err;
            auto success = query(& rep, pragma, & err);

            if (!success) {
                sqlite3_close_v2(rep.dbh);
                rep.dbh = nullptr;

                pfs::throw_or(perr, std::move(err));
                return rep_type{};
            }
        }
    }

    return rep;
}

bool database::wipe (fs::path const & path, error * perr)
{
    std::error_code ec;

    if (fs::exists(path, ec) && fs::is_regular_file(path, ec))
        fs::remove(path, ec);

    if (ec) {
        pfs::throw_or(perr, ec, tr::_("wipe Sqlite3 database"), fs::utf8_encode(path));
        return false;
    }

    return true;
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
relational_database<BACKEND>::query (std::string const & sql, error * perr)
{
    backend::sqlite3::query(& _rep, sql, perr);
}

template <>
void
relational_database<BACKEND>::begin (error * perr)
{
    query("BEGIN TRANSACTION", perr);
}

template <>
void
relational_database<BACKEND>::commit (error * perr)
{
    query("COMMIT TRANSACTION", perr);
    sqlite3_wal_checkpoint_v2(_rep.dbh, nullptr, SQLITE_CHECKPOINT_FULL, nullptr, nullptr);
}

template <>
void
relational_database<BACKEND>::rollback (error * perr)
{
    query("ROLLBACK TRANSACTION", perr);
}

template <>
relational_database<BACKEND>::statement_type
relational_database<BACKEND>::prepare (std::string const & sql, bool cache, error * perr)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER);

    auto pos = _rep.cache.find(sql);

    // Found in cache
    if (pos != _rep.cache.end()) {
        sqlite3_reset(pos->second);
        sqlite3_clear_bindings(pos->second);
        return statement_type::make(pos->second, true);
    }

    struct sqlite3_stmt * sth {nullptr};

    auto rc = sqlite3_prepare_v2(_rep.dbh, sql.c_str()
        , static_cast<int>(sql.size()), & sth, nullptr);

    if (SQLITE_OK != rc) {
        error err {
              errc::sql_error
            , backend::sqlite3::build_errstr(rc, _rep.dbh)
            , sql
        };

        if (perr) {
            *perr = std::move(err);
            return statement_type::make(nullptr, false);
        } else {
            throw err;
        }
    }

    if (cache) {
        auto res = _rep.cache.emplace(sql, sth);
        PFS__ASSERT(res.second, "key must be unique");
    }

    return statement_type::make(sth, cache);
}

template <>
std::size_t
relational_database<BACKEND>::rows_count (std::string const & table_name, error * perr)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER);

    std::size_t count = 0;
    std::string sql = fmt::format("SELECT COUNT(1) as count FROM \"{}\""
        , table_name);

    statement_type stmt = prepare(sql, perr);

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
        PFS__ASSERT(res.is_done(), "expecting done");
    }

    return count;
}

template <>
std::vector<std::string>
relational_database<BACKEND>::tables (std::string const & pattern, error * perr)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER);

    std::string sql = std::string{"SELECT name FROM sqlite_master"
        " WHERE type='table' ORDER BY name"};

    auto stmt = prepare(sql, perr);
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

        PFS__ASSERT(res.is_done(), "expecting done");
    }

    return list;
}
template <>
void
relational_database<BACKEND>::clear (std::string const & table, error * perr)
{
    query(fmt::format("DELETE FROM \"{}\"", table), perr);
}

template <>
void
relational_database<BACKEND>::remove (std::vector<std::string> const & tables, error * perr)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER);

    if (tables.empty())
        return;

    try {
        begin();

        query("PRAGMA foreign_keys=OFF");

        for (auto const & name: tables) {
            auto sql = fmt::format("DROP TABLE IF EXISTS \"{}\"", name);
            query(sql);
        }

        query("PRAGMA foreign_keys=ON");
        commit();
    } catch (error const & ex) {
        rollback();

        if (perr)
            *perr = ex;
        else
            throw;
    }
}

template <>
void
relational_database<BACKEND>::remove_all (error * perr)
{
    error err;
    auto list = tables(std::string{}, & err);

    if (!err)
        remove(list, & err);

    if (err) {
        if (perr)
            *perr = std::move(err);
        else
            throw err;
    }
}

template <>
bool
relational_database<BACKEND>::exists (std::string const & name, error * perr)
{
    auto stmt = prepare(
        fmt::format("SELECT name FROM sqlite_master"
            " WHERE type='table' AND name='{}'"
            , name), perr);

    if (stmt) {
        auto res = stmt.exec();

        if (res.has_more())
            return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// Key-value database
//
namespace backend {
namespace sqlite3 {

database::rep_type
database::make_kv (pfs::filesystem::path const & path, bool create_if_missing, error * perr)
{
    auto rep = make_r(path, create_if_missing);

    std::string sql = "CREATE TABLE IF NOT EXISTS debby"
        " (key TEXT NOT NULL UNIQUE, value BLOB"
        " , PRIMARY KEY(key)) WITHOUT ROWID";

    error err;
    auto success = query(& rep, sql, & err);

    if (!success) {
        sqlite3_close_v2(rep.dbh);
        rep.dbh = nullptr;

        if (perr) {
            *perr = std::move(err);
            return rep_type{};
        } else {
            throw err;
        }
    }

    return rep;
}

static bool get (database::rep_type const * rep
    , database::key_type const & key
    , std::string * string_result
    , blob_t * blob_result
    , error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    error err;
    std::string sql = fmt::format("SELECT value FROM debby WHERE key = '{}'", key);
    sqlite3_stmt * stmt = nullptr;

    do {
        auto rc = sqlite3_prepare_v2(rep->dbh, sql.c_str(), static_cast<int>(sql.size())
            , & stmt, nullptr);

        if (rc != SQLITE_OK) {
            err = error {
                    errc::sql_error
                  , tr::f_("read failure for key: {}", build_errstr(rc, rep->dbh))
                  , sql
            };
            break;
        }

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_ROW) {
            err = error { errc::key_not_found, tr::f_("key not found: '{}'", key) };
            break;
        }

        if (string_result) {
            auto cstr = reinterpret_cast<char const *>(sqlite3_column_text(stmt, 0));
            int size = sqlite3_column_bytes(stmt, 0);
            *string_result = std::string(cstr, size);
        } else if (blob_result) {
            auto data = reinterpret_cast<char const *>(sqlite3_column_blob(stmt, 0));
            int size = sqlite3_column_bytes(stmt, 0);
            blob_result->resize(size);
            std::memcpy(blob_result->data(), data, size);
        }
    } while (false);

    if (stmt)
        sqlite3_finalize(stmt);

    if (err) {
        if (perr) {
            *perr = err;
            return false;
        } else {
            throw err;
        }
    }

    return true;
}

/**
 * Removes value for @a key.
 */
static bool remove (database::rep_type * rep, database::key_type const & key, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);
    std::string sql = fmt::format("DELETE FROM debby WHERE key = '{}'", key);
    return query(rep, sql, perr);
}

static bool put (database::rep_type * rep, database::key_type const & key
    , char const * data, std::size_t len, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    // Attempt to write `null` data interpreted as delete operation for key
    if (!data)
        return remove(rep, key, perr);

    error err;
    std::string sql {"INSERT OR REPLACE INTO debby (key, value) VALUES (?, ?)"};
    sqlite3_stmt * stmt = nullptr;

    do {
        auto rc = sqlite3_prepare_v2(rep->dbh, sql.c_str(), static_cast<int>(sql.size())
            , & stmt, nullptr);

        if (rc != SQLITE_OK) {
            err = error { errc::sql_error, build_errstr(rc, rep->dbh), sql };
            break;
        }

        sqlite3_bind_text(stmt, 1, key.c_str(), static_cast<int>(key.size()), SQLITE_STATIC);
        sqlite3_bind_blob(stmt, 2, data, static_cast<int>(len), SQLITE_STATIC);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE) {
            err = error { errc::sql_error, build_errstr(rc, stmt), current_sql(stmt) };
            break;
        }
    } while (false);

    if (stmt)
        sqlite3_finalize(stmt);

    if (err) {
        if (perr) {
            *perr = err;
            return false;
        } else {
            throw err;
        }
    }

    return true;
}

}} // namespace backend::sqlite3

template <>
keyvalue_database<BACKEND>::keyvalue_database (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
keyvalue_database<BACKEND>::keyvalue_database (keyvalue_database && other)
{
    _rep = std::move(other._rep);
    other._rep.dbh = nullptr;
}

template <>
keyvalue_database<BACKEND>::~keyvalue_database ()
{
    if (_rep.dbh)
        sqlite3_close_v2(_rep.dbh);

    _rep.dbh = nullptr;
}

template <>
keyvalue_database<BACKEND>::operator bool () const noexcept
{
    return _rep.dbh != nullptr;
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key
    , std::intmax_t value, std::size_t size, error * perr)
{
    char buf[sizeof(std::intmax_t)];
    backend::pack_arithmetic(buf, value, size);
    backend::sqlite3::put(& _rep, key, buf, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, double value
    , std::size_t size, error * perr)
{
    backend::fixed_packer<double> p;
    p.value = value;
    backend::sqlite3::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, float value
    , std::size_t size, error * perr)
{
    backend::fixed_packer<float> p;
    p.value = value;
    backend::sqlite3::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_chars (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    backend::sqlite3::put(& _rep, key, data, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_blob (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    backend::sqlite3::put(& _rep, key, data, size, perr);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::remove
////////////////////////////////////////////////////////////////////////////////
template <>
void
keyvalue_database<BACKEND>::remove (key_type const & key, error * perr)
{
    backend::sqlite3::remove(& _rep, key, perr);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::get
////////////////////////////////////////////////////////////////////////////////

template <>
std::intmax_t
keyvalue_database<BACKEND>::get_integer (key_type const & key, error * perr) const
{
    error err;
    blob_t blob;
    backend::sqlite3::get(& _rep, key, nullptr, & blob, & err);

    if (!err) {
        auto res = backend::get_integer(blob);

        if (res.first)
            return res.second;
    }

    if (!err)
        err = backend::make_unsuitable_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return std::intmax_t{0};
}

template <>
float keyvalue_database<BACKEND>::get_float (key_type const & key, error * perr) const
{
    error err;
    blob_t blob;
    backend::sqlite3::get(& _rep, key, nullptr, & blob, & err);

    if (!err) {
        auto res = backend::get_float(blob);

        if (res.first)
            return res.second;
    }

    if (!err)
        err = backend::make_unsuitable_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return float{0};
}

template <>
double keyvalue_database<BACKEND>::get_double (key_type const & key, error * perr) const
{
    error err;
    blob_t blob;
    backend::sqlite3::get(& _rep, key, nullptr, & blob, & err);

    if (!err) {
        auto res = backend::get_double(blob);

        if (res.first)
            return res.second;
    }

    if (!err)
        err = backend::make_unsuitable_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return double{0};
}

template <>
std::string keyvalue_database<BACKEND>:: get_string (key_type const & key, error * perr) const
{
    std::string s;
    backend::sqlite3::get(& _rep, key, & s, nullptr, perr);
    return s;
}

template <>
blob_t keyvalue_database<BACKEND>::get_blob (key_type const & key, error * perr) const
{
    blob_t blob;
    backend::sqlite3::get(& _rep, key, nullptr, & blob, perr);
    return blob;
}

} // namespace debby

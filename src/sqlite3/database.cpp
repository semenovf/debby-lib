////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2023.02.07 Applied new API.
//      2024.10.29 V2 started.
////////////////////////////////////////////////////////////////////////////////
#include "statement_impl.hpp"
#include "debby/relational_database.hpp"
#include "debby/sqlite3.hpp"
#include "sqlite3.h"
#include "utils.hpp"
#include "../database_common.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <regex>
#include <unordered_map>

namespace fs = pfs::filesystem;

DEBBY__NAMESPACE_BEGIN

using database_t = relational_database<backend_enum::sqlite3>;

template <>
class database_t::impl
{
public:
    using native_type = struct sqlite3 *;
    using cache_type = std::unordered_map<std::string, struct sqlite3_stmt *>;

private:
    native_type _dbh {nullptr};
    cache_type  _cache; // Prepared statements cache

public:
    impl (native_type dbh) : _dbh(dbh)
    {}

    impl (impl && d)
    {
        _dbh = d._dbh;
        d._dbh = nullptr;
        _cache = std::move(d._cache);
    }

    ~impl ()
    {
       // Finalize cached statements
        for (auto & x: _cache)
            sqlite3_finalize(x.second);

        _cache.clear();

        if (_dbh != nullptr)
            sqlite3_close_v2(_dbh);

        _dbh = nullptr;
    }

public:
    bool query (std::string const & sql, error * perr)
    {
        int rc = sqlite3_exec(_dbh, sql.c_str(), nullptr, nullptr, nullptr);

        if (SQLITE_OK != rc) {
            pfs::throw_or(perr, error {errc::sql_error, sqlite3::build_errstr(rc, _dbh), sql});
            return false;
        }

        return true;
    }

    database_t::statement_type prepare (std::string const & sql, bool cache, error * perr)
    {
        if (_dbh == nullptr)
            return database_t::statement_type{};

        auto pos = _cache.find(sql);

        // Found in cache
        if (pos != _cache.end()) {
            sqlite3_reset(pos->second);
            sqlite3_clear_bindings(pos->second);
            statement_t::impl d{pos->second, true};
            return database_t::statement_type {std::move(d)};
        }

        struct sqlite3_stmt * sth {nullptr};

        auto rc = sqlite3_prepare_v2(_dbh, sql.c_str(), static_cast<int>(sql.size()), & sth, nullptr);

        if (SQLITE_OK != rc) {
            pfs::throw_or(perr, error{errc::sql_error, sqlite3::build_errstr(rc, _dbh), sql});
            return database_t::statement_type{};
        }

        if (cache) {
            auto res = _cache.emplace(sql, sth);

            if (!res.second) {
                pfs::throw_or(perr, error{pfs::errc::unexpected_error, tr::_("key must be unique")});
                return database_t::statement_type{};
            }
        }

        statement_t::impl d{sth, cache};
        return database_t::statement_type{std::move(d)};
    }
};

template <>
database_t::relational_database () = default;

template <>
database_t::relational_database (impl && d)
{
    _d = new impl(std::move(d));
}

template <>
database_t::relational_database (database_t && other) noexcept
{
    _d = other._d;
    other._d = nullptr;
}

template <>
database_t::~relational_database ()
{
    if (_d != nullptr)
        delete _d;

    _d = nullptr;
}

namespace sqlite3 {

int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second

database_t make (fs::path const & path, bool create_if_missing, error * perr)
{
    return make(path, create_if_missing, make_options{}, perr);
}

database_t make (pfs::filesystem::path const & path, bool create_if_missing, preset_enum preset, error * perr)
{
    switch (preset) {
        // See https://phiresky.github.io/blog/2020/sqlite-performance-tuning/
        case CONCURRENCY_PRESET: {
            make_options opts;
            opts.pragma_journal_mode = JM_WAL;
            opts.pragma_synchronous = SYN_NORMAL;
            opts.pragma_temp_store = TS_MEMORY;
            opts.pragma_mmap_size = 30000000000UL;

            return make(path, create_if_missing, std::move(opts), perr);
        }

        case DEFAULT_PRESET:
        default:
            break;
    }

    return make(path, create_if_missing, make_options{}, perr);
}

database_t make (pfs::filesystem::path const & path, bool create_if_missing, make_options && opts, error * perr)
{
    struct sqlite3 * dbh = nullptr;

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

    PFS__TERMINATE(sqlite3_enable_shared_cache(0) == SQLITE_OK, "");

    auto utf8_path = fs::utf8_encode(path);

    int rc = sqlite3_open_v2(utf8_path.c_str(), & dbh, flags, default_vfs);

    if (rc != SQLITE_OK) {
        if (dbh == nullptr) {
            // Unable to allocate memory for database handler.
            // Internal error code.
            pfs::throw_or(perr, error {errc::bad_alloc});
        } else {
            sqlite3_close_v2(dbh);
            dbh = nullptr;

            bool is_special_file = path.empty()
                || *path.c_str() == PFS__LITERAL_PATH(':');

            errc e = errc::success;

            if (rc == SQLITE_CANTOPEN && !is_special_file) {
                e = errc::database_not_found;
            } else {
                e = errc::backend_error;
            }

            pfs::throw_or(perr, error { e, utf8_path, build_errstr(rc, dbh) });
        }
    } else {
        // NOTE what for this call ?
        sqlite3_busy_timeout(dbh, MAX_BUSY_TIMEOUT);

        // Enable extended result codes
        sqlite3_extended_result_codes(dbh, 1);

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

        database_t::impl d{dbh};

        for (auto const & pragma: pragmas) {
            error err;
            auto success = d.query(pragma, & err);

            if (!success) {
                pfs::throw_or(perr, std::move(err));
                return database_t{};
            }
        }

        return database_t{std::move(d)};
    }

    return database_t{};
}

bool wipe (fs::path const & path, error * perr)
{
    std::error_code ec;

    if (fs::exists(path, ec) && fs::is_regular_file(path, ec))
        fs::remove(path, ec);

    if (ec) {
        pfs::throw_or(perr, ec, tr::_("wipe sqlite3 database"), fs::utf8_encode(path));
        return false;
    }

    return true;
}

} // namespace sqlite3

template <>
void database_t::query (std::string const & sql, error * perr)
{
    _d->query(sql, perr);
}

template <>
void database_t::begin (error * perr)
{
    query("BEGIN TRANSACTION", perr);
}

template <>
void database_t::commit (error * perr)
{
    query("COMMIT TRANSACTION", perr);
    //sqlite3_wal_checkpoint_v2(_rep.dbh, nullptr, SQLITE_CHECKPOINT_FULL, nullptr, nullptr);
}

template <>
void database_t::rollback (error * perr)
{
    query("ROLLBACK TRANSACTION", perr);
}

template <>
database_t::statement_type database_t::prepare (std::string const & sql, bool cache, error * perr)
{
    if (_d == nullptr)
        return database_t::statement_type{};

    return _d->prepare(sql, cache, perr);
}

template <>
std::vector<std::string> database_t::tables (std::string const & pattern, error * perr)
{
    if (_d == nullptr)
        return std::vector<std::string>{};

    std::string sql = std::string{"SELECT name FROM sqlite_master"
        " WHERE type='table' ORDER BY name"};

    auto stmt = prepare(sql, false, perr);
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

        if (!res.is_done()) {
            pfs::throw_or(perr, error {pfs::errc::unexpected_error, tr::_("expecting done")});
            return std::vector<std::string>{};
        }
    }

    return list;
}

template <>
void database_t::clear (std::string const & table, error * perr)
{
    query(fmt::format("DELETE FROM \"{}\"", table), perr);
}

template <>
void database_t::remove (std::vector<std::string> const & tables, error * perr)
{
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
        pfs::throw_or(perr, error{ex});
    }
}

template <>
void database_t::remove_all (error * perr)
{
    error err;
    auto list = tables(std::string{}, & err);

    if (!err)
        remove(list, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return;
    }
}

template <>
bool database_t::exists (std::string const & name, error * perr)
{
    auto stmt = prepare(
        fmt::format("SELECT name FROM sqlite_master WHERE type='table' AND name='{}'", name)
            , false, perr);

    if (stmt) {
        auto res = stmt.exec();

        if (res.has_more())
            return true;
    }

    return false;
}

// Explicit instantiation
template database_t & database_t::operator = (relational_database && other);

template
std::size_t
database_t::rows_count (std::string const & table_name, error * perr);

DEBBY__NAMESPACE_END

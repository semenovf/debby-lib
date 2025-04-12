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
#include "../relational_database_common.hpp"
#include "relational_database_impl.hpp"
#include "debby/sqlite3.hpp"
#include <pfs/assert.hpp>
#include <regex>

namespace fs = pfs::filesystem;

DEBBY__NAMESPACE_BEGIN

template database_t::relational_database ();
template database_t::relational_database (impl && d) noexcept;
template database_t::relational_database (database_t && other) noexcept;
template database_t::~relational_database ();
template database_t & database_t::operator = (relational_database && other) noexcept;

template std::size_t database_t::rows_count (std::string const & table_name, error * perr);

template <>
void database_t::query (std::string const & sql, error * perr)
{
    _d->query(sql, perr);
}

template <>
database_t::result_type database_t::exec (std::string const & sql, error * perr)
{
    return _d->exec(sql, perr);
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
database_t::statement_type database_t::prepare (std::string const & sql, error * perr)
{
    if (!_d)
        return database_t::statement_type{};

    return _d->prepare(sql, false, perr);
}

template <>
database_t::statement_type database_t::prepare_cached (std::string const & sql, error * perr)
{
    if (!_d)
        return database_t::statement_type{};

    return _d->prepare(sql, true, perr);
}

template <>
std::vector<std::string> database_t::tables (std::string const & pattern, error * perr)
{
    if (!_d)
        return std::vector<std::string>{};

    std::string sql = std::string{"SELECT name FROM sqlite_master"
        " WHERE type='table' ORDER BY name"};

    std::vector<std::string> list;

    auto res = this->exec(sql, perr);

    if (res) {
        std::regex rx {pattern.empty() ? ".*" : pattern};

        while (res.has_more()) {
            auto opt_name = res.get<std::string>(0, perr);

            if (opt_name) {
                if (pattern.empty()) {
                    list.push_back(*opt_name);
                } else {
                    if (std::regex_search(*opt_name, rx))
                        list.push_back(*opt_name);
                }
            } else {
                pfs::throw_or(perr, error {
                      pfs::make_error_code(pfs::errc::unexpected_error)
                    , tr::_("expected table name")
                });

                return std::vector<std::string>{};
            }

            // May be `sql_error` exception
            res.next();
        }
    }

    if (!res.is_done()) {
        pfs::throw_or(perr, error {
              pfs::make_error_code(pfs::errc::unexpected_error)
            , tr::_("expecting done")
        });

        return std::vector<std::string>{};
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
    auto res = this->exec(fmt::format("SELECT name FROM sqlite_master WHERE type='table' AND name='{}'", name), perr);

    if (res.has_more())
        return true;

    return false;
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
            pfs::throw_or(perr, error {make_error_code(errc::bad_alloc)});
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

            pfs::throw_or(perr, error {
                   make_error_code(e)
                , fmt::format("{}: {}", utf8_path, build_errstr(rc, dbh))
            });
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
        pfs::throw_or(perr, ec, tr::f_("wipe sqlite3 database: {}", fs::utf8_encode(path)));
        return false;
    }

    return true;
}

} // namespace sqlite3

DEBBY__NAMESPACE_END

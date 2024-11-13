////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.13 Initial version (moved from relational_database.cpp).
////////////////////////////////////////////////////////////////////////////////
#include "statement_impl.hpp"
#include "debby/relational_database.hpp"
#include "sqlite3.h"
#include "utils.hpp"
#include <pfs/i18n.hpp>
#include <unordered_map>

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

    impl (impl && other) noexcept
    {
        _dbh = other._dbh;
        other._dbh = nullptr;
        _cache = std::move(other._cache);
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

DEBBY__NAMESPACE_END

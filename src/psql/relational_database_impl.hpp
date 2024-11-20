////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.14 Initial version (moved from relational_database.cpp).
////////////////////////////////////////////////////////////////////////////////
#include "debby/relational_database.hpp"
#include "result_impl.hpp"
#include "statement_impl.hpp"
#include "utils.hpp"
#include <pfs/i18n.hpp>
#include <pfs/string_view.hpp>

extern "C" {
#include <libpq-fe.h>
}

DEBBY__NAMESPACE_BEGIN

using database_t = relational_database<backend_enum::psql>;

template <>
class database_t::impl
{
public:
    using native_type = struct pg_conn *;

private:
    native_type _dbh {nullptr};

public:
    impl (native_type dbh) : _dbh(dbh)
    {}

    impl (impl && d) noexcept
    {
        _dbh = d._dbh;
        d._dbh = nullptr;
    }

    ~impl ()
    {
        if (_dbh)
            PQfinish(_dbh);

        _dbh = nullptr;
    }

public:
    database_t::result_type exec (std::string const & sql, error * perr)
    {
        PGresult * res = PQexec(_dbh, sql.c_str());

        if (res == nullptr) {
            pfs::throw_or(perr, error {
                  errc::sql_error
                , tr::f_("query execution failure: {}: {}", sql, psql::build_errstr(_dbh))
            });

            return database_t::result_type{};
        }

        ExecStatusType status = PQresultStatus(res);
        bool r = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);

        if (!r) {
            if (status == PGRES_FATAL_ERROR) {
                pfs::throw_or(perr, error {
                      errc::backend_error
                    , tr::f_("query failure : {}", psql::build_errstr(_dbh))
                });
            } else {
                pfs::throw_or(perr, error {
                      errc::sql_error
                    , tr::f_("query failure: query request should not return any data")
                });
            }

            PQclear(res);
            return database_t::result_type{};
        }

        result_type::impl d{res};
        return database_t::result_type{std::move(d)};
    }

    bool query (std::string const & sql, error * perr)
    {
        auto res = this->exec(sql, perr);
        return !!res;
    }

    database_t::statement_type prepare (std::string const & sql, error * perr)
    {
        if (_dbh == nullptr)
            return database_t::statement_type{};

        auto res = PQdescribePrepared(_dbh, sql.c_str());

        if (res) {
            ExecStatusType status = PQresultStatus(res);

            if (status == PGRES_COMMAND_OK) {
                statement_t::impl d{_dbh, sql};
                return database_t::statement_type {std::move(d)};
            }
        } else {
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::f_("check prepared statement existence failure: {}: {}"
                    , sql, psql::build_errstr(_dbh))
            });

            return database_t::statement_type{};
        }

        PGresult * sth = PQprepare(_dbh, sql.c_str(), sql.c_str(), 0, nullptr);

        if (sth == nullptr) {
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::f_("prepare statement failure: {}: {}", sql, psql::build_errstr(_dbh))
            });

            return database_t::statement_type{};
        }

        ExecStatusType status = PQresultStatus(sth);
        bool r = (status == PGRES_COMMAND_OK/* || status == PGRES_TUPLES_OK*/);

        PQclear(sth);

        if (!r) {
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::f_("query failure : {}", psql::build_errstr(_dbh))
            });

            return database_t::statement_type{};
        }

        statement_t::impl d{_dbh, sql};
        return database_t::statement_type{std::move(d)};
    }
};

DEBBY__NAMESPACE_END

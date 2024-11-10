////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
//      2024.11.02 V2 started.
////////////////////////////////////////////////////////////////////////////////
#include "statement_impl.hpp"
#include "utils.hpp"
#include "debby/psql.hpp"
#include "../database_common.hpp"
#include <pfs/fmt.hpp>
#include <pfs/i18n.hpp>
#include <pfs/string_view.hpp>
#include <regex>

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

    impl (impl && d)
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
    bool query (std::string const & sql, error * perr)
    {
        PGresult * res = PQexec(_dbh, sql.c_str());

        if (res == nullptr) {
            pfs::throw_or(perr, error {
                  errc::sql_error
                , tr::f_("query execution failure: {}: {}", sql, psql::build_errstr(_dbh))
            });

            return false;
        }

        ExecStatusType status = PQresultStatus(res);
        bool r = (status == PGRES_COMMAND_OK);

        PQclear(res);

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

            return false;
        }

        return true;
    }

    database_t::statement_type prepare (std::string const & sql, bool cache, error * perr)
    {
        if (_dbh == nullptr)
            return database_t::statement_type{};

        // Check already prepared
        if (cache) {
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
        }

        PGresult * sth = PQprepare(_dbh, cache ? sql.c_str() : "", sql.c_str(), 0, nullptr);

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

// Forward declaration
template <>
void database_t::query (std::string const & sql, error * perr);

namespace psql {

static PGconn * connect (std::string const & conninfo, error * perr)
{
    auto is_uri = pfs::starts_with(pfs::string_view{conninfo}, "postgresql:")
        || pfs::starts_with(pfs::string_view{conninfo}, "postgres:");

    if (is_uri) {
        ; // This is a stub for the code specific to URI. Nothing useful yet.
    }

    //
    // This function will always return a non-null object pointer,
    // unless perhaps there is too little memory even
    // to allocate the PGconn object. The PQstatus function should be
    // called to check the return value for a successful connection before
    // queries are sent via the connection object.
    //
    PGconn * dbh = PQconnectdb(conninfo.c_str());

    if (!dbh) {
        pfs::throw_or(perr, error {
              errc::bad_alloc
            , tr::f_("bad connection parameters or database URI: {}", conninfo)
        });

        return nullptr;
    } else if (PQstatus(dbh) != CONNECTION_OK) {
        // Note. Error string may contains "out of memory" if `conninfo` is incomplete.

        PQfinish(dbh);

        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::f_("database connection failure: {}: {}", conninfo, build_errstr(dbh))
        });

        return nullptr;
    }

    // CONNECTION_OK
    return dbh;
}

database_t make (std::string const & conninfo, error * perr)
{
    auto dbh = connect(conninfo, perr);
    return database_t{database_t::impl{dbh}};
}

database_t make (std::string const & conninfo, notice_processor_type proc , error * perr)
{
    error err;
    auto dbh = connect(conninfo, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        database_t{};
    }

    PQsetNoticeProcessor(dbh, proc, nullptr);
    return database_t{database_t::impl{dbh}};
}

bool wipe (std::string const & db_name, std::string const & conninfo, error * perr)
{
    error err;
    auto db = make(conninfo, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return false;
    }

    db.query(fmt::format("DROP DATABASE IF EXISTS {}", db_name), & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return false;
    }

    return true;
}

} // namespace psql

template <>
void database_t::query (std::string const & sql, error * perr)
{
    _d->query(sql, perr);
}

template <>
void database_t::begin (error * perr)
{
    query("BEGIN", perr);
}

template <>
void database_t::commit (error * perr)
{
    query("COMMIT", perr);
}

template <>
void database_t::rollback (error * perr)
{
    query("ROLLBACK", perr);
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

    // SQL statement borrowed from Qt5 project
    std::string sql = "SELECT pg_class.relname, pg_namespace.nspname from pg_class"
        " LEFT JOIN pg_namespace ON (pg_class.relnamespace = pg_namespace.oid)"
        " WHERE (pg_class.relkind = 'r') AND (pg_class.relname !~ '^Inv')"
        " AND (pg_class.relname !~ '^pg_')"
        " AND (pg_namespace.nspname != 'information_schema');";

    auto stmt = prepare(sql, true, perr);
    std::vector<std::string> list;

    if (stmt) {
        auto res = stmt.exec(perr);

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
    query(fmt::format("DELETE FROM '{}'", table), perr);
}

template <>
void database_t::remove (std::vector<std::string> const & tables, error * perr)
{
    if (tables.empty())
        return;

    try {
        begin();

        for (auto const & name: tables) {
            auto sql = fmt::format("ALTER TABLE \"{}\" DISABLE TRIGGER ALL"
                "; DROP TABLE IF EXISTS \"{}\"", name, name);
            query(sql);
        }

        commit();
    } catch (error ex) {
        rollback();
        pfs::throw_or(perr, std::move(ex));
    }
}

template <>
void database_t::remove_all (error * perr)
{
    error err;
    auto list = tables(std::string{}, & err);

    if (!err)
        remove(list, & err);

    if (err)
        pfs::throw_or(perr, std::move(err));
}

template <>
bool database_t::exists (std::string const & name, error * perr)
{
    auto stmt = prepare(fmt::format("SELECT relname FROM pg_class WHERE relname='{}'", name)
        , false, perr);

    if (stmt) {
        auto res = stmt.exec();

        if (res.has_more())
            return true;
    }

    return false;
}

template database_t & database_t::operator = (relational_database && other);

template
std::size_t
database_t::rows_count (std::string const & table_name, error * perr);

DEBBY__NAMESPACE_END

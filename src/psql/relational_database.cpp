////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
//      2024.11.02 V2 started.
////////////////////////////////////////////////////////////////////////////////
#include "../relational_database_common.hpp"
#include "relational_database_impl.hpp"
#include "debby/psql.hpp"
#include <pfs/fmt.hpp>
#include <regex>
#include <vector>

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
database_t::statement_type database_t::prepare (std::string const & sql, error * perr)
{
    if (_d == nullptr)
        return database_t::statement_type{};

    return _d->prepare(sql, false, perr);
}

template <>
database_t::statement_type database_t::prepare_cached (std::string const & sql, error * perr)
{
    if (_d == nullptr)
        return database_t::statement_type{};

    return _d->prepare(sql, true, perr);
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

    std::vector<std::string> list;

    auto res = this->exec(sql, perr);

    if (res) {
        std::regex rx {pattern.empty() ? ".*" : pattern};

        while (res.has_more()) {
            auto opt_name = res.get<std::string>(1);

            if (pattern.empty()) {
                list.push_back(*opt_name);
            } else {
                if (std::regex_search(*opt_name, rx))
                    list.push_back(*opt_name);
            }

            // May be `sql_error` exception
            res.next();
        }
    }

    if (!res.is_done()) {
        pfs::throw_or(perr, pfs::make_error_code(pfs::errc::unexpected_error));
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
    auto res = this->exec(fmt::format("SELECT relname FROM pg_class WHERE relname='{}'", name), perr);

    if (res.has_more())
        return true;

    return false;
}

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
        pfs::throw_or(perr, make_error_code(errc::bad_alloc)
            , tr::f_("bad connection parameters or database URI: {}", conninfo));

        return nullptr;
    } else if (PQstatus(dbh) != CONNECTION_OK) {
        // Note. Error string may contains "out of memory" if `conninfo` is incomplete.

        PQfinish(dbh);

        pfs::throw_or(perr, make_error_code(errc::backend_error)
            , tr::f_("database connection failure: {}: {}", conninfo, psql::build_errstr(dbh)));

        return nullptr;
    }

    // CONNECTION_OK
    return dbh;
}

database_t make (std::string const & conninfo, error * perr)
{
    auto dbh = connect(conninfo, perr);
    return dbh == nullptr ? database_t{} : database_t{database_t::impl{dbh}};
}

database_t make (std::string const & conninfo, notice_processor_type proc , error * perr)
{
    error err;
    auto dbh = connect(conninfo, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return database_t{};
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

DEBBY__NAMESPACE_END

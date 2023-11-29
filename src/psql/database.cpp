////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/i18n.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/relational_database.hpp"
#include "pfs/debby/backend/psql/database.hpp"
#include "../kv_common.hpp"
#include "utils.hpp"
#include <regex>

extern "C" {
#include <libpq-fe.h>
}

namespace debby {

// int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second
static char const * NULL_HANDLER_TEXT = tr::noop_("uninitialized database handler");

namespace backend {
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

static bool query (database::rep_type const * rep, std::string const & sql, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER_TEXT);

    PGresult * res = PQexec(rep->dbh, sql.c_str());

    if (res == nullptr) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::f_("query execution failure: {}: {}", sql, backend::psql::build_errstr(rep->dbh))
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
                , tr::f_("query failure : {}", build_errstr(rep->dbh))
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

database::rep_type
database::make_r (std::string const & conninfo, error * perr)
{
    rep_type rep;
    rep.dbh = connect(conninfo, perr);
    return rep;
}


database::rep_type
database::make_r (std::string const & conninfo, notice_processor_type proc, error * perr)
{
    error err;
    auto rep = make_r(conninfo, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return rep;
    }

    PQsetNoticeProcessor(rep.dbh, proc, nullptr);

    return rep;
}

bool database::wipe (std::string const & db_name, std::string const & conninfo, error * perr)
{
    error err;
    auto rep = make_r(conninfo, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return false;
    }

    return query(& rep, fmt::format("DROP DATABASE IF EXISTS {}", db_name), perr);
}

}} // namespace backend::psql

using BACKEND = backend::psql::database;

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
    if (_rep.dbh)
        PQfinish(_rep.dbh);

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
    backend::psql::query(& _rep, sql, perr);
}

template <>
void
relational_database<BACKEND>::begin (error * perr)
{
    query("BEGIN", perr);
}

template <>
void
relational_database<BACKEND>::commit (error * perr)
{
    query("COMMIT", perr);
}

template <>
void
relational_database<BACKEND>::rollback (error * perr)
{
    query("ROLLBACK", perr);
}

template <>
relational_database<BACKEND>::statement_type
relational_database<BACKEND>::prepare (std::string const & sql, bool cache, error * perr)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER_TEXT);

    // Check already prepared
    if (cache) {
        auto res = PQdescribePrepared(_rep.dbh, sql.c_str());

        if (res) {
            ExecStatusType status = PQresultStatus(res);

            if (status == PGRES_COMMAND_OK)
                return statement_type::make(_rep.dbh, sql);
        } else {
            pfs::throw_or(perr, error {
                    errc::backend_error
                , tr::f_("check prepared statement existence failure: {}: {}"
                    , sql, backend::psql::build_errstr(_rep.dbh))
            });
        }
    }

    PGresult * sth = PQprepare(_rep.dbh, cache ? sql.c_str() : "", sql.c_str(), 0, nullptr);

    if (sth == nullptr) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::f_("prepare statement failure: {}: {}", sql, backend::psql::build_errstr(_rep.dbh))
        });

        return statement_type::make(nullptr, std::string{});
    }

    ExecStatusType status = PQresultStatus(sth);
    bool r = (status == PGRES_COMMAND_OK/* || status == PGRES_TUPLES_OK*/);

    PQclear(sth);

    if (!r) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::f_("query failure : {}", backend::psql::build_errstr(_rep.dbh))
        });

        return statement_type::make(nullptr, std::string{});
    }

    return statement_type::make(_rep.dbh, cache ? sql : "");
}

template <>
std::size_t
relational_database<BACKEND>::rows_count (std::string const & table_name, error * perr)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER_TEXT);

    std::size_t count = 0;
    std::string sql = fmt::format("SELECT COUNT(1) as count FROM `{}`", table_name);

    statement_type stmt = prepare(sql, false, perr);

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
    PFS__ASSERT(_rep.dbh, NULL_HANDLER_TEXT);

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

        PFS__ASSERT(res.is_done(), "expecting done");
    }

    return list;
}

template <>
void
relational_database<BACKEND>::clear (std::string const & table, error * perr)
{
    query(fmt::format("DELETE FROM '{}'", table), perr);
}

template <>
void
relational_database<BACKEND>::remove (std::vector<std::string> const & tables, error * perr)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER_TEXT);

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

    if (err)
        pfs::throw_or(perr, std::move(err));
}

template <>
bool
relational_database<BACKEND>::exists (std::string const & name, error * perr)
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

////////////////////////////////////////////////////////////////////////////////
// Key-value database
//
namespace backend {
namespace psql {

database::rep_type
database::make_kv (std::string const & conninfo, error * perr)
{
    error err;
    auto rep = make_r(conninfo, & err);

    if (err) {
        pfs::throw_or(perr, err);
        return rep;
    }

    // FIXME
//     std::string sql = "CREATE TABLE IF NOT EXISTS `debby`"
//         " (`key` TEXT NOT NULL UNIQUE, `value` BLOB"
//         " , PRIMARY KEY(`key`)) WITHOUT ROWID";
//
//     error err;
//     auto success = query(& rep, sql, & err);
//
//     if (!success) {
//         sqlite3_close_v2(rep.dbh);
//         rep.dbh = nullptr;
//
//         if (perr) {
//             *perr = std::move(err);
//             return rep_type{};
//         } else {
//             throw err;
//         }
//     }

    return rep;
}

database::rep_type
database::make_kv (std::string const & conninfo, notice_processor_type proc, error * perr)
{
    error err;
    auto rep = make_r(conninfo, proc, & err);

    if (err) {
        pfs::throw_or(perr, std::move(err));
        return rep;
    }

    return rep;
}

// FIXME
// static bool get (database::rep_type const * rep
//     , database::key_type const & key
//     , std::string * string_result
//     , blob_t * blob_result
//     , error * perr)
// {
//     PFS__ASSERT(rep->dbh, NULL_HANDLER_TEXT);
//
//     error err;
//     std::string sql = fmt::format("SELECT `value` FROM `debby` WHERE `key` = '{}'", key);
//     sqlite3_stmt * stmt = nullptr;
//
//     do {
//         auto rc = sqlite3_prepare_v2(rep->dbh, sql.c_str(), static_cast<int>(sql.size())
//             , & stmt, nullptr);
//
//         if (rc != SQLITE_OK) {
//             err = error {
//                     errc::sql_error
//                   , tr::f_("read failure for key: {}", build_errstr(rc, rep->dbh))
//                   , sql
//             };
//             break;
//         }
//
//         rc = sqlite3_step(stmt);
//
//         if (rc != SQLITE_ROW) {
//             err = error { errc::key_not_found, tr::f_("key not found: '{}'", key) };
//             break;
//         }
//
//         if (string_result) {
//             auto cstr = reinterpret_cast<char const *>(sqlite3_column_text(stmt, 0));
//             int size = sqlite3_column_bytes(stmt, 0);
//             *string_result = std::string(cstr, size);
//         } else if (blob_result) {
//             auto data = reinterpret_cast<char const *>(sqlite3_column_blob(stmt, 0));
//             int size = sqlite3_column_bytes(stmt, 0);
//             blob_result->resize(size);
//             std::memcpy(blob_result->data(), data, size);
//         }
//     } while (false);
//
//     if (stmt)
//         sqlite3_finalize(stmt);
//
//     if (err) {
//         if (perr) {
//             *perr = err;
//             return false;
//         } else {
//             throw err;
//         }
//     }
//
//     return true;
// }
//
// /**
//  * Removes value for @a key.
//  */
// static bool remove (database::rep_type * rep, database::key_type const & key, error * perr)
// {
//     PFS__ASSERT(rep->dbh, NULL_HANDLER_TEXT);
//     std::string sql = fmt::format("DELETE FROM `debby` WHERE `key` = '{}'", key);
//     return query(rep, sql, perr);
// }
//
// static bool put (database::rep_type * rep, database::key_type const & key
//     , char const * data, std::size_t len, error * perr)
// {
//     PFS__ASSERT(rep->dbh, NULL_HANDLER_TEXT);
//
//     // Attempt to write `null` data interpreted as delete operation for key
//     if (!data)
//         return remove(rep, key, perr);
//
//     error err;
//     std::string sql {"INSERT OR REPLACE INTO `debby` (`key`, `value`) VALUES (?, ?)"};
//     sqlite3_stmt * stmt = nullptr;
//
//     do {
//         auto rc = sqlite3_prepare_v2(rep->dbh, sql.c_str(), static_cast<int>(sql.size())
//             , & stmt, nullptr);
//
//         if (rc != SQLITE_OK) {
//             err = error { errc::sql_error, build_errstr(rc, rep->dbh), sql };
//             break;
//         }
//
//         sqlite3_bind_text(stmt, 1, key.c_str(), static_cast<int>(key.size()), SQLITE_STATIC);
//         sqlite3_bind_blob(stmt, 2, data, static_cast<int>(len), SQLITE_STATIC);
//
//         rc = sqlite3_step(stmt);
//
//         if (rc != SQLITE_DONE) {
//             err = error { errc::sql_error, build_errstr(rc, stmt), current_sql(stmt) };
//             break;
//         }
//     } while (false);
//
//     if (stmt)
//         sqlite3_finalize(stmt);
//
//     if (err) {
//         if (perr) {
//             *perr = err;
//             return false;
//         } else {
//             throw err;
//         }
//     }
//
//     return true;
// }

}} // namespace backend::psql

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
        PQfinish(_rep.dbh);

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
    // FIXME
//     char buf[sizeof(std::intmax_t)];
//     backend::pack_arithmetic(buf, value, size);
//     backend::sqlite3::put(& _rep, key, buf, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, double value
    , std::size_t size, error * perr)
{
    // FIXME
//     backend::fixed_packer<double> p;
//     p.value = value;
//     backend::sqlite3::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, float value
    , std::size_t size, error * perr)
{
    // FIXME
//     backend::fixed_packer<float> p;
//     p.value = value;
//     backend::sqlite3::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_chars (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    // FIXME
//     backend::sqlite3::put(& _rep, key, data, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_blob (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    // FIXME
//     backend::sqlite3::put(& _rep, key, data, size, perr);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::remove
////////////////////////////////////////////////////////////////////////////////
template <>
void
keyvalue_database<BACKEND>::remove (key_type const & key, error * perr)
{
    // FIXME
//     backend::sqlite3::remove(& _rep, key, perr);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::get
////////////////////////////////////////////////////////////////////////////////

template <>
std::intmax_t
keyvalue_database<BACKEND>::get_integer (key_type const & key, error * perr) const
{
    // FIXME
//     error err;
//     blob_t blob;
//     backend::sqlite3::get(& _rep, key, nullptr, & blob, & err);
//
//     if (!err) {
//         auto res = backend::get_integer(blob);
//
//         if (res.first)
//             return res.second;
//     }
//
//     if (!err)
//         err = backend::make_unsuitable_error(key);
//
//     if (perr)
//         *perr = err;
//     else
//         throw err;

    return std::intmax_t{0};
}

template <>
float keyvalue_database<BACKEND>::get_float (key_type const & key, error * perr) const
{
        // FIXME
//     error err;
//     blob_t blob;
//     backend::sqlite3::get(& _rep, key, nullptr, & blob, & err);
//
//     if (!err) {
//         auto res = backend::get_float(blob);
//
//         if (res.first)
//             return res.second;
//     }
//
//     if (!err)
//         err = backend::make_unsuitable_error(key);
//
//     if (perr)
//         *perr = err;
//     else
//         throw err;

    return float{0};
}

template <>
double keyvalue_database<BACKEND>::get_double (key_type const & key, error * perr) const
{
        // FIXME
//     error err;
//     blob_t blob;
//     backend::sqlite3::get(& _rep, key, nullptr, & blob, & err);
//
//     if (!err) {
//         auto res = backend::get_double(blob);
//
//         if (res.first)
//             return res.second;
//     }
//
//     if (!err)
//         err = backend::make_unsuitable_error(key);
//
//     if (perr)
//         *perr = err;
//     else
//         throw err;

    return double{0};
}

template <>
std::string keyvalue_database<BACKEND>:: get_string (key_type const & key, error * perr) const
{
    std::string s;
    // FIXME
//     backend::sqlite3::get(& _rep, key, & s, nullptr, perr);
    return s;
}

template <>
blob_t keyvalue_database<BACKEND>::get_blob (key_type const & key, error * perr) const
{
    blob_t blob;
        // FIXME
//     backend::sqlite3::get(& _rep, key, nullptr, & blob, perr);
    return blob;
}

} // namespace debby

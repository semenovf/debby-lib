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
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/keyvalue_database.hpp"
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
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

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
database::make_r (fs::path const & path, bool create_if_missing)
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
    auto utf8_path = pfs::filesystem::utf8_encode(path);
    int rc = sqlite3_open_v2(utf8_path.c_str(), & rep.dbh, flags, default_vfs);
#else
    int rc = sqlite3_open_v2(path.c_str(), & rep.dbh, flags, default_vfs);
#endif

    std::error_code ec;

    if (rc != SQLITE_OK) {
        if (!rep.dbh) {
            // Unable to allocate memory for database handler.
            // Internal error code.
            throw error {
                make_error_code(errc::bad_alloc)
            };
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

            throw error {
                  ec
                , fs::utf8_encode(path)
                , build_errstr(rc, rep.dbh)
            };
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
            throw res;
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
        throw res;
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
        throw error {
              make_error_code(errc::sql_error)
            , backend::sqlite3::build_errstr(rc, _rep.dbh)
            , sql
        };
    }

    if (cache) {
        auto res = _rep.cache.emplace(sql, sth);
        PFS__ASSERT(res.second, "key must be unique");
    }

    return statement_type::make(sth, cache);
}

template <>
std::size_t
relational_database<BACKEND>::rows_count (std::string const & table_name)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER);

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
        PFS__ASSERT(res.is_done(), "expecting done");
    }

    return count;
}

template <>
std::vector<std::string>
relational_database<BACKEND>::tables (std::string const & pattern)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER);

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

        PFS__ASSERT(res.is_done(), "expecting done");
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
    PFS__ASSERT(_rep.dbh, NULL_HANDLER);

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
        throw;
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

////////////////////////////////////////////////////////////////////////////////
// Key-value database
//
namespace backend {
namespace sqlite3 {

database::rep_type
database::make_kv (pfs::filesystem::path const & path, bool create_if_missing)
{
    auto rep = make_r(path, create_if_missing);

    std::string sql = "CREATE TABLE IF NOT EXISTS `debby`"
        " (`key` TEXT NOT NULL UNIQUE, `type` INTEGER NOT NULL, `value` BLOB"
        " , PRIMARY KEY(`key`)) WITHOUT ROWID";

    auto res = query(& rep, sql);

    if (!res.ok()) {
        sqlite3_close_v2(rep.dbh);
        rep.dbh = nullptr;
        throw res;
    }

    return rep;
}

/**
 * Removes value for @a key.
 */
static result_status remove (database::rep_type * rep, database::key_type const & key)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    std::string sql = fmt::format("DELETE FROM `debby` WHERE `key` = '{}'", key);

    return query(rep, sql);
}

result_status database::rep_type::write (
      database::rep_type * rep
    , database::key_type const & key
    , int column_family_index
    , char const * data, std::size_t len)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    std::string sql {"INSERT OR REPLACE INTO `debby`"
        " (`key`, `type`, `value`) VALUES (?, ?, ?)"};

    sqlite3_stmt * stmt = nullptr;
    auto rc = sqlite3_prepare_v2(rep->dbh, sql.c_str(), static_cast<int>(sql.size())
        , & stmt, nullptr);

    if (SQLITE_OK != rc) {
        auto err = error{make_error_code(errc::sql_error)
            , build_errstr(rc, rep->dbh)
            , sql
        };

        return err;
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), static_cast<int>(key.size()), SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, column_family_index);
    sqlite3_bind_blob(stmt, 3, data, static_cast<int>(len), SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        auto err = error{
              make_error_code(errc::sql_error)
            , build_errstr(rc, stmt)
            , current_sql(stmt)
        };

        return err;
    }

    sqlite3_finalize(stmt);
    return result_status{};
}

static result_status read (database::rep_type const * rep
    , database::key_type const & key
    , int & column_family_index
    , std::string & target)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    std::string sql = fmt::format("SELECT `type`, `value` FROM `debby`"
        " WHERE `key` = '{}'", key);

    sqlite3_stmt * stmt = nullptr;
    auto rc = sqlite3_prepare_v2(rep->dbh, sql.c_str(), static_cast<int>(sql.size())
        , & stmt, nullptr);

    if (SQLITE_OK != rc) {
        auto err = error{make_error_code(errc::sql_error)
            , build_errstr(rc, rep->dbh)
            , sql
        };

        return err;
    }

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        auto err = error{
              make_error_code(errc::key_not_found)
            , fmt::format("key not found: '{}'", key)
        };

        return err;
    }

    column_family_index = sqlite3_column_int(stmt, 0);

    auto cstr = reinterpret_cast<char const *>(sqlite3_column_text(stmt, 1));
    int size = sqlite3_column_bytes(stmt, 1);
    target = std::string(cstr, size);

    sqlite3_finalize(stmt);

    return result_status{};
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
void
keyvalue_database<BACKEND>::destroy ()
{
    // TODO DEPRECATED This method will be removed later.
}

// TODO This method is equivalent to `backend::rocksdb::fetch`, must be optimized.
template <>
result_status
keyvalue_database<BACKEND>::fetch (key_type const & key
    , keyvalue_database<BACKEND>::value_type & value) const noexcept
{
    int column_family_index = -1;
    std::string target;

    auto res = backend::sqlite3::read(& _rep, key, column_family_index, target);

    if (!res.ok()) {
        value = value_type{nullptr};
        return res;
    }

    bool corrupted = false;

    if (target.size() == 0) {
        switch (column_family_index) {
            case backend::sqlite3::database::BLOB_COLUMN_FAMILY_INDEX:
                value = value_type{blob_t{}};
                return result_status{};
            case backend::sqlite3::database::STR_COLUMN_FAMILY_INDEX:
                value = value_type{std::string{}};
                return result_status{};

            case backend::sqlite3::database::INT_COLUMN_FAMILY_INDEX:
            case backend::sqlite3::database::FP_COLUMN_FAMILY_INDEX:
            default:
                corrupted = true;
                break;
        }
    }

    auto len = target.size();

    if (!corrupted) {
        switch (column_family_index) {
            case backend::sqlite3::database::INT_COLUMN_FAMILY_INDEX: {
                std::intmax_t intmax_value{0};

                switch(len) {
                    case sizeof(std::int8_t): {
                        backend::sqlite3::database::fixed_packer<std::int8_t> p;
                        std::memcpy(p.bytes, target.data(), len);
                        intmax_value = static_cast<std::intmax_t>(p.value);
                        break;
                    }

                    case sizeof(std::int16_t): {
                        backend::sqlite3::database::fixed_packer<std::int16_t> p;
                        std::memcpy(p.bytes, target.data(), len);
                        intmax_value = static_cast<std::intmax_t>(p.value);
                        break;
                    }

                    case sizeof(std::int32_t): {
                        backend::sqlite3::database::fixed_packer<std::int32_t> p;
                        std::memcpy(p.bytes, target.data(), len);
                        intmax_value = static_cast<std::intmax_t>(p.value);
                        break;
                    }

                    case sizeof(std::int64_t): {
                        backend::sqlite3::database::fixed_packer<std::int64_t> p;
                        std::memcpy(p.bytes, target.data(), len);
                        intmax_value = static_cast<std::intmax_t>(p.value);
                        break;
                    }

                    default:
                        corrupted = true;
                        break;
                }

                if (!corrupted) {
                    if (pfs::holds_alternative<bool>(value)) {
                        value = static_cast<bool>(intmax_value);
                    } else if (pfs::holds_alternative<double>(value)) {
                        value = static_cast<double>(intmax_value);
                    } else if (pfs::holds_alternative<blob_t>(value)) {
                        blob_t blob(sizeof(intmax_value));
                        std::memcpy(blob.data(), & intmax_value, sizeof(intmax_value));
                        value = std::move(blob);
                    } else if (pfs::holds_alternative<std::string>(value)) {
                        std::string s = std::to_string(intmax_value);
                        value = std::move(s);
                    } else { // std::nullptr_t, std::intmax_t
                        value = intmax_value;
                    }

                    return result_status{};
                }

                break;
            }

            case backend::sqlite3::database::FP_COLUMN_FAMILY_INDEX: {
                double double_value{0};

                switch(len) {
                    case sizeof(float): {
                        backend::sqlite3::database::fixed_packer<float> p;
                        std::memcpy(p.bytes, target.data(), len);
                        double_value = static_cast<double>(p.value);
                        break;
                    }

                    case sizeof(double): {
                        backend::sqlite3::database::fixed_packer<double> p;
                        std::memcpy(p.bytes, target.data(), len);
                        double_value = static_cast<double>(p.value);
                        break;
                    }

                    default:
                        corrupted = true;
                        break;
                }

                if (!corrupted) {
                    if (pfs::holds_alternative<bool>(value)) {
                        value = static_cast<bool>(double_value);
                    } else if (pfs::holds_alternative<std::intmax_t>(value)) {
                        value = static_cast<std::intmax_t>(double_value);
                    } else if (pfs::holds_alternative<blob_t>(value)) {
                        blob_t blob(sizeof(double));
                        std::memcpy(blob.data(), & double_value, sizeof(double_value));
                        value = std::move(blob);
                    } else if (pfs::holds_alternative<std::string>(value)) {
                        std::string s = std::to_string(double_value);
                        value = std::move(s);
                    } else { // std::nullptr_t, double
                        value = double_value;
                    }

                    return result_status{};
                }

                break;
            }

            case backend::sqlite3::database::BLOB_COLUMN_FAMILY_INDEX: {
                blob_t blob;
                blob.resize(len);
                std::memcpy(blob.data(), target.data(), len);
                value = value_type{std::move(blob)};
                return result_status{};
            }

            case backend::sqlite3::database::STR_COLUMN_FAMILY_INDEX:
                value = value_type{std::move(target)};
                return result_status{};

            default:
                corrupted = true;
                break;
        }
    }

    if (corrupted) {
        error err {make_error_code(errc::bad_value)
            , fmt::format("unsuitable or corrupted data stored"
                " by key: {}", key)
        };

        return err;
    }

    return result_status{};
}

template <>
void
keyvalue_database<BACKEND>::set (key_type const & key, std::string const & value)
{
    auto err = rep_type::write(& _rep, key
        , backend::sqlite3::database::STR_COLUMN_FAMILY_INDEX
        , value.data(), value.size());

    if (err)
        throw err;
}

template <>
void
keyvalue_database<BACKEND>::set (key_type const & key
    , char const * value, std::size_t len)
{
    auto err = rep_type::write(& _rep, key
        , backend::sqlite3::database::STR_COLUMN_FAMILY_INDEX
        , value, len);

    if (err)
        throw err;
}

template <>
void
keyvalue_database<BACKEND>::set (key_type const & key, blob_t const & value)
{
    auto err = rep_type::write(& _rep, key
        , backend::sqlite3::database::BLOB_COLUMN_FAMILY_INDEX
        , reinterpret_cast<char const *>(value.data()), value.size());

    if (err)
        throw err;
}

template <>
void
keyvalue_database<BACKEND>::remove (key_type const & key)
{
    backend::sqlite3::remove(& _rep, key);
}

} // namespace debby

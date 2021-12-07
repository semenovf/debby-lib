////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/debby/rocksdb/database.hpp"
#include <rocksdb/rocksdb_namespace.h>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include "pfs/fmt.hpp"

// #include <regex>
// #include <cassert>

namespace pfs {
namespace debby {
namespace rocksdb {

namespace {
// int constexpr MAX_BUSY_TIMEOUT = 1000; // 1 second
//
std::string ALREADY_OPEN_ERROR {"database is already open"};
std::string OPEN_ERROR         {"create/open database failure `{}`: {}"};
// std::string ALLOC_MEMORY_ERROR  {"unable to allocate memory for database handler"};

} // namespace

// PFS_DEBBY__EXPORT bool database::query_impl (std::string const & sql)
// {
//     assert(_dbh);
//
//     char * errmsg {nullptr};
//     int rc = sqlite3_exec(_dbh, sql.c_str(), nullptr, nullptr, & errmsg);
//
//     if (SQLITE_OK != rc) {
//         if (errmsg)
//             _last_error = errmsg;
//         else
//             _last_error = "sqlite3_exec() failure";
//
//         return false;
//     }
//
//     return true;
// }

PFS_DEBBY__EXPORT bool database::open_impl (filesystem::path const & path)
{
    if (_dbh) {
        _last_error = ALREADY_OPEN_ERROR;
        return false;
    }

    ::rocksdb::Options options;

    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();

    // Aggressively check consistency of the data.
    options.paranoid_checks = true;

    if (filesystem::exists(path)) {
        options.create_if_missing = false;
    } else {
        // Create the DB if it's not already present.
        options.create_if_missing = true;
    }

    // Open DB.
    // `path` is the path to a directory containing multiple database files
    ::rocksdb::Status status = ::rocksdb::DB::Open(options, path, & _dbh);

    if (!status.ok()) {
        _last_error = fmt::format(OPEN_ERROR, path.native(), status.ToString());
        return false;
    }

    // Database just created
//     if (options.create_if_missing) {
//         // ...
//     }

    return true;
}

PFS_DEBBY__EXPORT void database::close_impl ()
{
//     // Finalize cached statements
//     for (auto & item: _cache)
//         sqlite3_finalize(item.second);
//
//     _cache.clear();
//
//     if (_dbh)
//         sqlite3_close_v2(_dbh);

    _dbh = nullptr;
};

// PFS_DEBBY__EXPORT bool database::clear_impl ()
// {
//     if (!_dbh) {
//         _last_error = NOT_OPEN_ERROR;
//         return false;
//     }
//
//     auto list = tables_impl(std::string{});
//     auto success = query("PRAGMA foreign_keys = OFF");
//
//     if (!success)
//         return false;
//
//     for (auto const & t: list) {
//         auto sql = fmt::format("DROP TABLE IF EXISTS `{}`", t);
//         success = query(sql);
//
//         if (!success)
//             break;
//     }
//
//     success = query("PRAGMA foreign_keys = ON") && success;
//
//     return success;
// }
//
// PFS_DEBBY__EXPORT std::vector<std::string> database::tables_impl (std::string const & pattern)
// {
//     statement_type stmt = prepare(
//         "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
//
//     std::vector<std::string> list;
//     std::regex rx {pattern.empty() ? ".*" : pattern};
//
//     if (stmt) {
//         auto res = stmt.exec();
//
//         while (res.has_more()) {
//             auto value = res.fetch(0);
//
//             auto success = value.has_value() && holds_alternative<std::string>(*value);
//             assert(success);
//
//             if (pattern.empty()) {
//                 list.push_back(get<std::string>(*value));
//             } else {
//                 if (std::regex_search(get<std::string>(*value), rx))
//                     list.push_back(get<std::string>(*value));
//             }
//
//             res.next();
//         }
//
//         if (res.is_error()) {
//             _last_error = res.last_error();
//             return std::vector<std::string>{};
//         } else {
//             assert(res.is_done());
//         }
//     }
//
//     return list;
// }
//
// PFS_DEBBY__EXPORT bool database::exists_impl (std::string const & name)
// {
//     statement_type stmt = prepare(
//         fmt::format("SELECT name FROM sqlite_master"
//             " WHERE type='table' AND name='{}'"
//             , name));
//
//     if (stmt) {
//         auto res = stmt.exec();
//
//         if (res.has_more())
//             return true;
//     }
//
//     return false;
// }
//
// PFS_DEBBY__EXPORT statement database::prepare_impl (std::string const & sql, bool cache)
// {
//     if (!_dbh) {
//         _last_error = NOT_OPEN_ERROR;
//         return statement{};
//     }
//
//     auto pos = _cache.find(sql);
//
//     // Found in cache
//     if (pos != _cache.end()) {
//         sqlite3_reset(pos->second);
//         sqlite3_clear_bindings(pos->second);
//         return statement{pos->second, true};
//     }
//
//     statement_type::native_type sth {nullptr};
//
//     auto rc = sqlite3_prepare_v2(_dbh, sql.c_str(), sql.size(), & sth, nullptr);
//
//     if (SQLITE_OK != rc) {
//         _last_error = sqlite3_errmsg(_dbh);
//         assert(sth == nullptr);
//         return statement{};
//     }
//
//     if (cache) {
//         auto res = _cache.emplace(sql, sth);
//         assert(res.second); // key must be unique
//     }
//
//     return statement{sth, cache};
// }
//
// PFS_DEBBY__EXPORT bool database::begin_impl ()
// {
//     return query_impl("BEGIN TRANSACTION");
// }
//
// PFS_DEBBY__EXPORT bool database::commit_impl ()
// {
//     return query_impl("COMMIT TRANSACTION");
// }
//
// PFS_DEBBY__EXPORT bool database::rollback_impl ()
// {
//     return query_impl("ROLLBACK TRANSACTION");
// }

}}} // namespace pfs::debby::rocksdb

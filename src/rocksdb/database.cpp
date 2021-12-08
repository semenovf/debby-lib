////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/fmt.hpp"
#include "pfs/debby/rocksdb/database.hpp"
#include <rocksdb/rocksdb_namespace.h>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

namespace pfs {
namespace debby {
namespace rocksdb {

namespace {

std::string ALREADY_OPEN_ERROR {"database is already open"};
std::string OPEN_ERROR         {"create/open database failure `{}`: {}"};

} // namespace

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
    // if (options.create_if_missing) {
    //      // ...
    // }

    return true;
}

PFS_DEBBY__EXPORT void database::close_impl ()
{
    if (_dbh)
        delete _dbh;

    _dbh = nullptr;
};

PFS_DEBBY__EXPORT bool database::write (key_type const & key
    , char const * data, std::size_t len)
{
    auto status = _dbh->Put(::rocksdb::WriteOptions(), key, ::rocksdb::Slice(data, len));

    if (status.ok())
        return true;

    _last_error = fmt::format("failed to store value by key `{}': {}"
        , key
        , status.ToString());

    return false;
}

PFS_DEBBY__EXPORT database::expected_type<std::string, bool>
database::read (key_type const & key)
{
    std::string s;
    auto status = _dbh->Get(::rocksdb::ReadOptions(), key, & s);

    if (!status.ok()) {
        if (status.IsNotFound()) {
            return make_unexpected(false);
        } else {
            _last_error = fmt::format("failed to fetch value by key `{}': {}"
                , key, status.ToString());
            return make_unexpected(true);
        }
    }

    return s;
}

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

}}} // namespace pfs::debby::rocksdb

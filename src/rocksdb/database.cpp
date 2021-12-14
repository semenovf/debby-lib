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

std::string const database::ERROR_DOMAIN {"ROCKSDB"};

bool database::open_impl (filesystem::path const & path, bool create_if_missing)
{
    if (_dbh) {
        PFS_DEBBY_THROW((runtime_error{
              ERROR_DOMAIN
            , fmt::format("database is already open: {}", path.c_str())
        }));

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
        options.create_if_missing = create_if_missing;
    }

    // Open DB.
    // `path` is the path to a directory containing multiple database files
    ::rocksdb::Status status = ::rocksdb::DB::Open(options, path, & _dbh);

    if (!status.ok()) {
        PFS_DEBBY_THROW((runtime_error{
              ERROR_DOMAIN
            , fmt::format("create/open database failure: {}", path.c_str())
            , status.ToString()
        }));

        return false;
    }

    // Database just created
    // if (options.create_if_missing) {
    //      // ...
    // }

    return true;
}

void database::close_impl ()
{
    if (_dbh)
        delete _dbh;

    _dbh = nullptr;
};

bool database::write (key_type const & key
    , char const * data, std::size_t len)
{
    // Attempt to write `null` data interpreted as delete operation for key
    if (data) {
        auto status = _dbh->Put(::rocksdb::WriteOptions(), key, ::rocksdb::Slice(data, len));

        if (status.ok())
            return true;

        PFS_DEBBY_THROW((runtime_error{
              ERROR_DOMAIN
            , fmt::format("failed to store value by key: '{}'", key)
            , status.ToString()
        }));
    } else {
        return remove_impl(key);
    }

    return false;
}

optional<std::string> database::read (key_type const & key)
{
    std::string s;
    auto status = _dbh->Get(::rocksdb::ReadOptions(), key, & s);

    if (!status.ok()) {
        if (status.IsNotFound()) {
            return nullopt;
        } else {
            PFS_DEBBY_THROW((runtime_error{
                ERROR_DOMAIN
                , fmt::format("failed to fetch value by key: '{}'", key)
                , status.ToString()
            }));
        }
    }

    return s;
}

bool database::remove_impl (key_type const & key)
{
    auto status = _dbh->SingleDelete(::rocksdb::WriteOptions(), key);

    if (!status.ok()) {
        if (!status.IsNotFound()) {
            PFS_DEBBY_THROW((runtime_error{
                ERROR_DOMAIN
                , fmt::format("failed to remove value by key '{}'", key)
                , status.ToString()
            }));

            return false;
        }
    }

    return true;
}

}}} // namespace pfs::debby::rocksdb

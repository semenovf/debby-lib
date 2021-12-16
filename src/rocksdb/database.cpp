////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
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

std::error_code
database::open_impl (filesystem::path const & path, bool create_if_missing)
{
    if (_dbh) {
        auto rc = make_error_code(errc::database_already_open);
        exception::failure(exception{rc, path});
        return rc;
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

    // NOTE
    // Rocks's `CreateDirIfMissing()` regardless of `options.create_if_missing`
    // value.
    // There is the issue `rocksdb creates LOCK and LOG files even if DB
    // does not exist, and create_if_missing is false #5029`
    // (https://github.com/facebook/rocksdb/issues/5029).
    // Release 6.26.1 does not contain the required fixes yet.
    // See appropriate code at `db/db_impl/db_impl_open.cc`, method
    // `Status DBImpl::Open(const DBOptions& db_options...`.
    // Need to use workaround:
    if (!filesystem::exists(path) && !options.create_if_missing) {
        auto rc = make_error_code(errc::database_not_found);
        exception::failure(exception{rc, path});
        return rc;
    }

    // Open DB.
    // `path` is the path to a directory containing multiple database files
    ::rocksdb::Status status = ::rocksdb::DB::Open(options, path, & _dbh);

    if (!status.ok()) {
        auto rc = make_error_code(errc::backend_error);
        exception::failure(exception{rc
            , fmt::format("{} database failure: {}"
                , create_if_missing ? "create/open" : "open"
                , path.c_str())
            , status.ToString()
        });

        return rc;
    }

    // Database just created
    // if (options.create_if_missing) {
    //      // ...
    // }

    return std::error_code{};
}

std::error_code database::close_impl ()
{
    if (_dbh)
        delete _dbh;

    _dbh = nullptr;

    return std::error_code{};
};

std::error_code database::write (key_type const & key
    , char const * data, std::size_t len)
{
    // Attempt to write `null` data interpreted as delete operation for key
    if (data) {
        auto status = _dbh->Put(::rocksdb::WriteOptions(), key, ::rocksdb::Slice(data, len));

        if (!status.ok()) {
            auto rc = make_error_code(errc::backend_error);
            exception::failure(exception{rc
                , fmt::format("write failure for key: '{}'", key)
                , status.ToString()
            });

            return rc;
        }
    } else {
        return remove_impl(key);
    }

    return std::error_code{};
}

optional<std::string> database::read (key_type const & key, std::error_code & ec)
{
    std::string s;
    auto status = _dbh->Get(::rocksdb::ReadOptions(), key, & s);

    if (!status.ok()) {
        if (status.IsNotFound()) {
            return nullopt;
        } else {
            ec = make_error_code(errc::backend_error);

            exception::failure(exception{ec
                , fmt::format("read failure for key: '{}'", key)
                , status.ToString()
            });

            return nullopt;
        }
    }

    return s;
}

std::error_code database::remove_impl (key_type const & key)
{
    auto status = _dbh->SingleDelete(::rocksdb::WriteOptions(), key);

    if (!status.ok()) {
        if (!status.IsNotFound()) {
            auto rc = make_error_code(errc::backend_error);
            exception::failure(exception{rc
                , fmt::format("remove failure for key: '{}'", key)
                , status.ToString()
            });

            return rc;
        }
    }

    return std::error_code{};
}

}}} // namespace pfs::debby::rocksdb

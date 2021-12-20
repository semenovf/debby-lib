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

namespace debby {
namespace rocksdb {

namespace {
char const * NULL_HANDLER = "uninitialized database handler";
} // namespace

bool database::open (pfs::filesystem::path const & path
    , bool create_if_missing
    , error * perr) noexcept
{
    DEBBY__ASSERT(!_dbh, NULL_HANDLER);

    ::rocksdb::Options options;

    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();

    // Aggressively check consistency of the data.
    options.paranoid_checks = true;

    if (pfs::filesystem::exists(path)) {
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
    if (!pfs::filesystem::exists(path) && !options.create_if_missing) {
        auto ec = make_error_code(errc::database_not_found);
        auto err = error{ec, PFS_UTF8_ENCODE_PATH(path.c_str())};
        if (perr) *perr = err; else DEBBY__THROW(err);

        return false;
    }

    // Open DB.
    // `path` is the path to a directory containing multiple database files
    ::rocksdb::Status status = ::rocksdb::DB::Open(options, path, & _dbh);

    if (!status.ok()) {
        auto ec = make_error_code(errc::backend_error);
        auto err = error{ec
            , PFS_UTF8_ENCODE_PATH(path.c_str())
            , status.ToString()};
        if (perr) *perr = err; else DEBBY__THROW(err);

        return false;
    }

    // Database just created
    // if (options.create_if_missing) {
    //      // ...
    // }

    return true;
}

void database::close () noexcept
{
    if (_dbh)
        delete _dbh;

    _dbh = nullptr;
};

bool database::write (key_type const & key
    , char const * data, std::size_t len
    , error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    // Attempt to write `null` data interpreted as delete operation for key
    if (data) {
        auto status = _dbh->Put(::rocksdb::WriteOptions(), key, ::rocksdb::Slice(data, len));

        if (!status.ok()) {
            auto ec = make_error_code(errc::backend_error);
            auto err = error{ec
                , fmt::format("write failure for key: '{}'", key)
                , status.ToString()};
            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        }
    } else {
        return remove_impl(key, perr);
    }

    return true;
}

bool database::read (key_type const & key, pfs::optional<std::string> & target
    , error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    std::string s;
    auto status = _dbh->Get(::rocksdb::ReadOptions(), key, & s);

    if (!status.ok()) {
        if (status.IsNotFound()) {
            target = pfs::nullopt;
            return true;
        } else {
            auto ec = make_error_code(errc::backend_error);
            auto err = error{ec
                , fmt::format("read failure for key: '{}'", key)
                , status.ToString()};
            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        }
    }

    target = std::move(s);
    return true;
}

bool database::remove_impl (key_type const & key, error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    auto status = _dbh->SingleDelete(::rocksdb::WriteOptions(), key);

    if (!status.ok()) {
        if (!status.IsNotFound()) {
            auto ec = make_error_code(errc::backend_error);
            auto err = error{ec
                , fmt::format("remove failure for key: '{}'", key)
                , status.ToString()};
            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        }
    }

    return true;
}

}} // namespace debby::rocksdb

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2023.02.08 Applied new API.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/filesystem.hpp"
#include "pfs/i18n.hpp"
#include "pfs/assert.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/backend/rocksdb/database.hpp"
#include "../kv_common.hpp"
#include <rocksdb/rocksdb_namespace.h>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

namespace fs = pfs::filesystem;

namespace debby {

namespace backend {
namespace rocksdb {

static char const * NULL_HANDLER = tr::noop_("uninitialized database handler");

static ::rocksdb::Options default_options ()
{
    ::rocksdb::Options options;

    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
    //options.IncreaseParallelism();
    //options.OptimizeLevelStyleCompaction();

    // Use this if your DB is very small (like under 1GB) and you don't want to
    // spend lots of memory for memtables.
    // options.OptimizeForSmallDb();

    // If true, the database will be created if it is missing.
    // Default: false
    options.create_if_missing = true;

    // Aggressively check consistency of the data.
    options.paranoid_checks = true;

    // Maximal info log files to be kept.
    // Default: 1000
    options.keep_log_file_num = 10;

    // If true, missing column families will be automatically created.
    // Default: false
    //options.create_missing_column_families = true;

    return options;
}

static bool get (database::rep_type const * rep
    , database::key_type const & key, std::string * string_result
    , blob_t * blob_result, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    std::string buf;
    ::rocksdb::Status status = rep->dbh->Get(::rocksdb::ReadOptions()
        , key, & buf);

    if (status.ok()) {
        if (string_result) {
            *string_result = std::move(buf);
            return true;
        }

        if (blob_result) {
            blob_result->resize(buf.size());
            std::memcpy(blob_result->data(), buf.data(), buf.size());
            return true;
        }
    }

    if (!status.ok()) {
        error err {
              status.IsNotFound()
                ? errc::key_not_found
                : errc::backend_error
            , status.IsNotFound()
                ? tr::f_("key not found: '{}'", key)
                : tr::f_("read failure for key: '{}'", key)
            , status.IsNotFound()
                ? ""
                : status.ToString()
        };

        if (perr)
            *perr = err;
        else
            throw err;
    }

    return false;
}

/**
 * Removes value for @a key.
 */
static bool remove (database::rep_type * rep, database::key_type const & key
    , error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    // Remove the database entry (if any) for "key". Returns OK on success,
    // and a non-OK status on error. It is not an error if "key" did not exist
    // in the database.
    // Note: consider setting options.sync = true.
    ::rocksdb::WriteOptions write_opts;
    write_opts.sync = true;
    ::rocksdb::Status status = rep->dbh->Delete(write_opts, key);

    if (!status.ok()) {
        if (!status.IsNotFound()) {
            error err {
                  errc::backend_error
                , tr::f_("remove failure for key: '{}'", key)
                , status.ToString()
            };

            if (perr) {
                *perr = err;
                return false;
            } else {
                throw err;
            }
        }
    }

    return true;
}

/**
 * Writes @c data into database by @a key.
 *
 * Attempt to write @c nullptr data interpreted as delete operation for key.
 *
 * @return @c result_status
 */
static bool put (database::rep_type * rep, database::key_type const & key
    , char const * data, std::size_t len, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    // Attempt to write `null` data interpreted as delete operation for key
    if (!data)
        return remove(rep, key, perr);

    auto status = rep->dbh->Put(::rocksdb::WriteOptions()
        , key, ::rocksdb::Slice(data, len));

    if (!status.ok()) {
        error err {
              errc::backend_error
            , tr::f_("write failure for key: '{}'", key)
            , status.ToString()
        };

        if (perr) {
            *perr = err;
            return false;
        } else {
            throw err;
        }
    }

    return true;
}

database::rep_type
database::make_kv (pfs::filesystem::path const & path, options_type * opts, error * perr)
{
    rep_type rep;
    rep.dbh = nullptr;

    ::rocksdb::Options default_opts = default_options();

    if (!opts) {
        opts = & default_opts;

        if (fs::exists(path)) {
            opts->create_if_missing = false;
        }
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
    if (!fs::exists(path) && !opts->create_if_missing) {
        throw error {
              make_error_code(errc::database_not_found)
            , fs::utf8_encode(path)
        };
    }

    ::rocksdb::Status status;

    // Open DB.
    // `path` is the path to a directory containing multiple database files
    //
    // ATTENTION! ROCKSDB_LITE causes a segmentaion fault while open the database
    status = ::rocksdb::DB::Open(*opts, fs::utf8_encode(path), & rep.dbh);

    if (!status.ok()) {
        error err {
              errc::backend_error
            , fs::utf8_encode(path)
            , status.ToString()
        };

        if (perr) {
            *perr = err;
            return rep_type{};
        } else {
            throw err;
        }
    }

    rep.path = path;

    return rep;
}

database::rep_type
database::make_kv (pfs::filesystem::path const & path, bool create_if_missing
    , bool optimize_for_small_db, error * perr)
{
    auto opts = default_options();
    opts.create_if_missing = create_if_missing;

    if (optimize_for_small_db)
        opts.OptimizeForSmallDb();

    return make_kv(path, & opts, perr);
}

bool database::wipe (fs::path const & path, error * perr)
{
    std::error_code ec;

    if (fs::exists(path, ec) && fs::is_directory(path, ec))
        fs::remove_all(path, ec);

    if (ec) {
        pfs::throw_or(perr, ec, tr::_("wipe RocksDB database"), fs::utf8_encode(path));
        return false;
    }

    return true;
}

}} // namespace backend::rocksdb

#define BACKEND backend::rocksdb::database

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
    if (_rep.dbh) {
        // _rep.dbh->FlushWAL(true  /* sync */);
        // ::rocksdb::FlushOptions options;
        // options.wait = true;
        //_rep.dbh->CancelAllBackgroundWork(true  /* wait */);

        _rep.dbh->Close();
        delete _rep.dbh;
        _rep.dbh = nullptr;
    }

    _rep.path.clear();
}

template <>
keyvalue_database<BACKEND>::operator bool () const noexcept
{
    return _rep.dbh != nullptr;
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, std::intmax_t value
    , std::size_t size, error * perr)
{
    char buf[sizeof(std::intmax_t)];
    backend::pack_arithmetic(buf, value, size);
    backend::rocksdb::put(& _rep, key, buf, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, double value
    , std::size_t size, error * perr)
{
    backend::fixed_packer<double> p;
    p.value = value;
    backend::rocksdb::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, float value
    , std::size_t size, error * perr)
{
    backend::fixed_packer<float> p;
    p.value = value;
    backend::rocksdb::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_chars (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    backend::rocksdb::put(& _rep, key, data, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_blob (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    backend::rocksdb::put(& _rep, key, data, size, perr);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::remove
////////////////////////////////////////////////////////////////////////////////
template <>
void
keyvalue_database<BACKEND>::remove (key_type const & key, error * perr)
{
    backend::rocksdb::remove(& _rep, key, perr);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::get
////////////////////////////////////////////////////////////////////////////////
template <>
std::intmax_t
keyvalue_database<BACKEND>::get_integer (key_type const & key, error * perr) const
{
    error err;
    blob_t blob;
    backend::rocksdb::get(& _rep, key, nullptr, & blob, & err);

    if (!err) {
        auto res = backend::get_integer(blob);

        if (res.first)
            return res.second;
    }

    if (!err)
        err = backend::make_unsuitable_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return std::intmax_t{0};
}

template <>
float keyvalue_database<BACKEND>::get_float (key_type const & key, error * perr) const
{
    error err;
    blob_t blob;
    backend::rocksdb::get(& _rep, key, nullptr, & blob, & err);

    if (!err) {
        auto res = backend::get_float(blob);

        if (res.first)
            return res.second;
    }

    if (!err)
        err = backend::make_unsuitable_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return float{0};
}

template <>
double keyvalue_database<BACKEND>::get_double (key_type const & key, error * perr) const
{
    error err;
    blob_t blob;
    backend::rocksdb::get(& _rep, key, nullptr, & blob, & err);

    if (!err) {
        auto res = backend::get_double(blob);

        if (res.first)
            return res.second;
    }

    if (!err)
        err = backend::make_unsuitable_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return double{0};
}

template <>
std::string keyvalue_database<BACKEND>::get_string (key_type const & key, error * perr) const
{
    std::string s;
    backend::rocksdb::get(& _rep, key, & s, nullptr, perr);
    return s;
}

template <>
blob_t keyvalue_database<BACKEND>::get_blob (key_type const & key, error * perr) const
{
    blob_t blob;
    backend::rocksdb::get(& _rep, key, nullptr, & blob, perr);
    return blob;
}

} // namespace debby

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

namespace fs = pfs::filesystem;

namespace {

char const * NULL_HANDLER = "uninitialized database handler";

} // namespace

database::database (pfs::filesystem::path const & path
    , bool create_if_missing
    , error * perr)
{
    auto opts = default_options();
    opts.create_if_missing = create_if_missing;
    open(path, & opts, perr);
}

::rocksdb::Options database::default_options ()
{
    ::rocksdb::Options options;

    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();

    // If true, the database will be created if it is missing.
    // Default: false
    options.create_if_missing = true;

    // Aggressively check consistency of the data.
    options.paranoid_checks = true;

    // Maximal info log files to be kept.
    // Default: 1000
    options.keep_log_file_num = 100;

    // If true, missing column families will be automatically created.
    // Default: false
    options.create_missing_column_families = true;

    return options;
}

bool database::open (pfs::filesystem::path const & path
    , ::rocksdb::Options * opts
    , error * perr) noexcept
{
    DEBBY__ASSERT(!_dbh, NULL_HANDLER);

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
        auto ec = make_error_code(errc::database_not_found);
        auto err = error{ec, fs::utf8_encode(path)};
        if (perr) *perr = err; else DEBBY__THROW(err);

        return false;
    }

    ::rocksdb::Status status;

    std::vector<::rocksdb::ColumnFamilyDescriptor> column_family_names = {
          ::rocksdb::ColumnFamilyDescriptor{"debby_cf_integers", ::rocksdb::ColumnFamilyOptions{}}
        , ::rocksdb::ColumnFamilyDescriptor{"debby_cf_floating_points", ::rocksdb::ColumnFamilyOptions{}}
        , ::rocksdb::ColumnFamilyDescriptor{"debby_cf_strings", ::rocksdb::ColumnFamilyOptions{}}
        , ::rocksdb::ColumnFamilyDescriptor{"debby_cf_blobs", ::rocksdb::ColumnFamilyOptions{}}
    };

    // Open DB.
    // `path` is the path to a directory containing multiple database files
    column_family_names.emplace_back(::rocksdb::kDefaultColumnFamilyName
        , ::rocksdb::ColumnFamilyOptions{});

    status = ::rocksdb::DB::Open(*opts, path
        , column_family_names, & _type_column_families, & _dbh);

    if (status.ok()) {
        if (_type_column_families.empty())
            status = _dbh->CreateColumnFamilies(column_family_names
                , & _type_column_families);
    }

    if (!status.ok()) {
        auto ec = make_error_code(errc::backend_error);
        auto err = error{ec
            , fs::utf8_encode(path)
            , status.ToString()};

        if (perr)
            *perr = err;
        else
            DEBBY__THROW(err);

        return false;
    }

    _path = path;

    return true;
}

void database::close () noexcept
{
    if (_dbh) {
        for (auto handle: _type_column_families) {
            _dbh->DestroyColumnFamilyHandle(handle);
        }

        delete _dbh;
    }

    _path.clear();
    _dbh = nullptr;
};

bool database::clear_impl (error * perr)
{
    if (!_path.empty() && pfs::filesystem::exists(_path)) {
        auto status = ::rocksdb::DestroyDB(_path, default_options());

        if (!status.ok()) {
            auto ec = make_error_code(errc::backend_error);
            auto err = error{ec
                , fmt::format("drop/clear database failure: {}"
                    , fs::utf8_encode(_path)
                    , status.ToString())};
            if (perr) *perr = err; else DEBBY__THROW(err);
            return false;
        }
    }

    return true;
}

bool database::write (key_type const & key
    , int column_family_index
    , char const * data, std::size_t len
    , error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    // Attempt to write `null` data interpreted as delete operation for key
    if (data) {
        auto status = _dbh->Put(::rocksdb::WriteOptions()
            , _type_column_families[column_family_index]
            , key, ::rocksdb::Slice(data, len));

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

bool database::read (key_type const & key, int & column_family_index
    , pfs::optional<std::string> & target
    , error * perr) const
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    std::string s;

    ::rocksdb::Status status;

    if (column_family_index < 0) {
        column_family_index = -1;

        for (auto handle: _type_column_families) {
            column_family_index++;
            status = _dbh->Get(::rocksdb::ReadOptions(), handle, key, & s);

            // Found
            if (status.ok())
                break;

            // Error
            if (!status.ok() && !status.IsNotFound()) {
                column_family_index = -1;
                break;
            }
        }
    } else {
        status = _dbh->Get(::rocksdb::ReadOptions()
            , _type_column_families[column_family_index]
            , key, & s);
    }

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

database::value_type database::fetch_impl (key_type const & key
    , int column_family_index
    , error * perr) const
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    pfs::optional<std::string> opt;

    if (!read(key, column_family_index, opt, perr))
        return value_type{nullptr};

    if (!opt)
        return value_type{nullptr};

    bool corrupted = false;

    if (opt->size() == 0) {
        switch (column_family_index) {
            case BLOB_COLUMN_FAMILY_INDEX:
                return value_type{blob_t{}};
            case STR_COLUMN_FAMILY_INDEX:
                return value_type{std::string{}};

            case INT_COLUMN_FAMILY_INDEX:
            case FP_COLUMN_FAMILY_INDEX:
            default:
                corrupted = true;
                break;
        }
    }

    int len = opt->size();

    if (!corrupted) {
        switch (column_family_index) {
            case INT_COLUMN_FAMILY_INDEX: {
                switch(len) {
                    case sizeof(std::int8_t): {
                        fixed_packer<std::int8_t> p;
                        std::memcpy(p.bytes, opt->data(), len);
                        return value_type{static_cast<std::intmax_t>(p.value)};
                    }
                    case sizeof(std::int16_t): {
                        fixed_packer<std::int16_t> p;
                        std::memcpy(p.bytes, opt->data(), len);
                        return value_type{static_cast<std::intmax_t>(p.value)};
                    }
                    case sizeof(std::int32_t): {
                        fixed_packer<std::int32_t> p;
                        std::memcpy(p.bytes, opt->data(), len);
                        return value_type{static_cast<std::intmax_t>(p.value)};
                    }
                    case sizeof(std::int64_t): {
                        fixed_packer<std::int64_t> p;
                        std::memcpy(p.bytes, opt->data(), len);
                        return value_type{static_cast<std::intmax_t>(p.value)};
                    }
                    default:
                        corrupted = true;
                        break;
                }
                break;
            }
            case FP_COLUMN_FAMILY_INDEX: {
                switch(len) {
                    case sizeof(float): {
                        fixed_packer<float> p;
                        std::memcpy(p.bytes, opt->data(), len);
                        return value_type{static_cast<double>(p.value)};
                    }
                    case sizeof(double): {
                        fixed_packer<double> p;
                        std::memcpy(p.bytes, opt->data(), len);
                        return value_type{static_cast<double>(p.value)};
                    }
                    default:
                        corrupted = true;
                        break;
                }
                break;
            }

            case BLOB_COLUMN_FAMILY_INDEX: {
                blob_t blob;
                blob.resize(len);
                std::memcpy(blob.data(), opt->data(), len);
                return value_type{std::move(blob)};
            }

            case STR_COLUMN_FAMILY_INDEX:
                return value_type{std::move(*opt)};

            default:
                corrupted = true;
                break;
        }
    }

    if (corrupted) {
        auto ec = make_error_code(errc::bad_value);
        auto err = error{ec, fmt::format("unsuitable or corrupted data stored"
            " by key: {}", key)};
        if (perr) *perr = err; else DEBBY__THROW(err);
    }

    return value_type{nullptr};
}

bool database::remove_impl (key_type const & key, error * perr)
{
    DEBBY__ASSERT(_dbh, NULL_HANDLER);

    ::rocksdb::Status status; // = _dbh->SingleDelete(::rocksdb::WriteOptions(), key);

    for (auto handle: _type_column_families) {
        status = _dbh->SingleDelete(::rocksdb::WriteOptions(), handle, key);

        if (!status.ok() && !status.IsNotFound())
            break;
    }

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

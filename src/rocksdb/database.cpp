////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/fmt.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/backend/rocksdb/database.hpp"
#include <rocksdb/rocksdb_namespace.h>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

namespace fs = pfs::filesystem;

namespace debby {

namespace backend {
namespace rocksdb {

static char const * NULL_HANDLER = "uninitialized database handler";

static ::rocksdb::Options default_options ()
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
    options.keep_log_file_num = 10;

    // If true, missing column families will be automatically created.
    // Default: false
    options.create_missing_column_families = true;

    return options;
}

static result_status read (database::rep_type const * rep
    , database::key_type const & key
    , int & column_family_index
    , std::string & target)
{
    DEBBY__ASSERT(rep->dbh, NULL_HANDLER);

    std::string s;

    ::rocksdb::Status status;

    if (column_family_index < 0) {
        column_family_index = -1;

        for (auto handle: rep->type_column_families) {
            column_family_index++;
            status = rep->dbh->Get(::rocksdb::ReadOptions(), handle, key, & s);

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
        status = rep->dbh->Get(::rocksdb::ReadOptions()
            , rep->type_column_families[column_family_index]
            , key, & s);
    }

    if (!status.ok()) {
        if (status.IsNotFound()) {
            auto err = error{
                  make_error_code(errc::key_not_found)
                , fmt::format("key not found: '{}'", key)
            };
            return err;
        } else {
            auto err = error{
                  make_error_code(errc::backend_error)
                , fmt::format("read failure for key: '{}'", key)
                , status.ToString()
            };
            return err;
        }
    }

    target = std::move(s);
    return result_status{};
}

/**
 * Removes value for @a key.
 */
static result_status remove (database::rep_type * rep, database::key_type const & key)
{
    DEBBY__ASSERT(rep->dbh, NULL_HANDLER);

    ::rocksdb::Status status; // = _dbh->SingleDelete(::rocksdb::WriteOptions(), key);

    for (auto handle: rep->type_column_families) {
        status = rep->dbh->SingleDelete(::rocksdb::WriteOptions(), handle, key);

        if (!status.ok() && !status.IsNotFound())
            break;
    }

    if (!status.ok()) {
        if (!status.IsNotFound()) {
            auto err = error{
                  make_error_code(errc::backend_error)
                , fmt::format("remove failure for key: '{}'", key)
                , status.ToString()};
            return err;
        }
    }

    return result_status{};
}

/**
 * Writes @c data into database by @a key.
 *
 * Attempt to write @c nullptr data interpreted as delete operation for key.
 *
 * @return @c result_status
 */
result_status database::rep_type::write (
      database::rep_type * rep
    , database::key_type const & key
    , int column_family_index
    , char const * data, std::size_t len)
{
    DEBBY__ASSERT(rep->dbh, NULL_HANDLER);

    // Attempt to write `null` data interpreted as delete operation for key
    if (data) {
        auto status = rep->dbh->Put(::rocksdb::WriteOptions()
            , rep->type_column_families[column_family_index]
            , key, ::rocksdb::Slice(data, len));

        if (!status.ok()) {
            auto err = error{
                  make_error_code(errc::backend_error)
                , fmt::format("write failure for key: '{}'", key)
                , status.ToString()
            };

            return err;
        }
    } else {
        return remove(rep, key);
    }

    return result_status{};
}

database::rep_type
database::make (pfs::filesystem::path const & path, options_type * opts)
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
        auto err = error{
              make_error_code(errc::database_not_found)
            , fs::utf8_encode(path)
        };
        DEBBY__THROW(err);
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

    status = ::rocksdb::DB::Open(*opts, fs::utf8_encode(path)
        , column_family_names, & rep.type_column_families, & rep.dbh);

    if (status.ok()) {
        if (rep.type_column_families.empty())
            status = rep.dbh->CreateColumnFamilies(column_family_names
                , & rep.type_column_families);
    }

    if (!status.ok()) {
        auto ec = make_error_code(errc::backend_error);
        auto err = error{ec, fs::utf8_encode(path), status.ToString()};
        DEBBY__THROW(err);
    }

    rep.path = path;

    return rep;
}

database::rep_type
database::make (pfs::filesystem::path const & path, bool create_if_missing)
{
    auto opts = default_options();
    opts.create_if_missing = create_if_missing;
    return make(path, & opts);
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
        for (auto handle: _rep.type_column_families)
            _rep.dbh->DestroyColumnFamilyHandle(handle);

        delete _rep.dbh;
    }

    _rep.path.clear();
    _rep.dbh = nullptr;
}

template <>
keyvalue_database<BACKEND>::operator bool () const noexcept
{
    return _rep.dbh != nullptr;
}

template <>
void
keyvalue_database<BACKEND>::clear ()
{
    if (!_rep.path.empty() && pfs::filesystem::exists(_rep.path)) {
        auto status = ::rocksdb::DestroyDB(fs::utf8_encode(_rep.path)
            , backend::rocksdb::default_options());

        if (!status.ok()) {
            auto ec = make_error_code(errc::backend_error);
            auto err = error{ec
                , fmt::format("drop/clear database failure: {}"
                    , fs::utf8_encode(_rep.path)
                    , status.ToString())};
            DEBBY__THROW(err);
        }
    }
}

template <>
void
keyvalue_database<BACKEND>::set (key_type const & key, std::string const & value)
{
    auto err = rep_type::write(& _rep, key
        , backend::rocksdb::database::STR_COLUMN_FAMILY_INDEX
        , value.data(), value.size());

    if (err)
        DEBBY__THROW(err);
}

template <>
void
keyvalue_database<BACKEND>::set (key_type const & key
    , char const * value, std::size_t len)
{
    auto err = rep_type::write(& _rep, key
        , backend::rocksdb::database::STR_COLUMN_FAMILY_INDEX
        , value, len);

    if (err)
        DEBBY__THROW(err);
}

template <>
void
keyvalue_database<BACKEND>::set (key_type const & key, blob_t const & value)
{
    auto err = rep_type::write(& _rep, key
        , backend::rocksdb::database::BLOB_COLUMN_FAMILY_INDEX
        , reinterpret_cast<char const *>(value.data()), value.size());

    if (err)
        DEBBY__THROW(err);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::remove
////////////////////////////////////////////////////////////////////////////////
template <>
void
keyvalue_database<BACKEND>::remove (key_type const & key)
{
    backend::rocksdb::remove(& _rep, key);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::remove
////////////////////////////////////////////////////////////////////////////////
template <>
result_status
keyvalue_database<BACKEND>::fetch (key_type const & key
    , keyvalue_database<BACKEND>::value_type & value) const noexcept
{
    int column_family_index = -1;
    std::string target;

    auto res = backend::rocksdb::read(& _rep, key, column_family_index, target);

    if (!res.ok()) {
        value = value_type{nullptr};
        return res;
    }

    bool corrupted = false;

    if (target.size() == 0) {
        switch (column_family_index) {
            case backend::rocksdb::database::BLOB_COLUMN_FAMILY_INDEX:
                value = value_type{blob_t{}};
                return result_status{};
            case backend::rocksdb::database::STR_COLUMN_FAMILY_INDEX:
                value = value_type{std::string{}};
                return result_status{};

            case backend::rocksdb::database::INT_COLUMN_FAMILY_INDEX:
            case backend::rocksdb::database::FP_COLUMN_FAMILY_INDEX:
            default:
                corrupted = true;
                break;
        }
    }

    auto len = target.size();

    if (!corrupted) {
        switch (column_family_index) {
            case backend::rocksdb::database::INT_COLUMN_FAMILY_INDEX: {
                std::intmax_t intmax_value{0};

                switch(len) {
                    case sizeof(std::int8_t): {
                        backend::rocksdb::database::fixed_packer<std::int8_t> p;
                        std::memcpy(p.bytes, target.data(), len);
                        intmax_value = static_cast<std::intmax_t>(p.value);
                        break;
                    }

                    case sizeof(std::int16_t): {
                        backend::rocksdb::database::fixed_packer<std::int16_t> p;
                        std::memcpy(p.bytes, target.data(), len);
                        intmax_value = static_cast<std::intmax_t>(p.value);
                        break;
                    }

                    case sizeof(std::int32_t): {
                        backend::rocksdb::database::fixed_packer<std::int32_t> p;
                        std::memcpy(p.bytes, target.data(), len);
                        intmax_value = static_cast<std::intmax_t>(p.value); 
                        break;
                    }

                    case sizeof(std::int64_t): {
                        backend::rocksdb::database::fixed_packer<std::int64_t> p;
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

            case backend::rocksdb::database::FP_COLUMN_FAMILY_INDEX: {
                double double_value{0};

                switch(len) {
                    case sizeof(float): {
                        backend::rocksdb::database::fixed_packer<float> p;
                        std::memcpy(p.bytes, target.data(), len);
                        double_value = static_cast<double>(p.value);
                        break;
                    }

                    case sizeof(double): {
                        backend::rocksdb::database::fixed_packer<double> p;
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

            case backend::rocksdb::database::BLOB_COLUMN_FAMILY_INDEX: {
                blob_t blob;
                blob.resize(len);
                std::memcpy(blob.data(), target.data(), len);
                value = value_type{std::move(blob)};
                return result_status{};
            }

            case backend::rocksdb::database::STR_COLUMN_FAMILY_INDEX:
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

} // namespace debby

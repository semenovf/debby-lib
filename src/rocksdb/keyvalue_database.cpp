////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2021.12.16 Reimplemented with new error handling.
//      2022.03.12 Refactored.
//      2023.02.08 Applied new API.
//      2024.11.10 V2 started.
////////////////////////////////////////////////////////////////////////////////
#include "../keyvalue_database_common.hpp"
#include "debby/keyvalue_database.hpp"
#include "debby/rocksdb.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/i18n.hpp>
#include <rocksdb/rocksdb_namespace.h>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <cstdint>

namespace fs = pfs::filesystem;

DEBBY__NAMESPACE_BEGIN

using keyvalue_database_t = keyvalue_database<backend_enum::rocksdb>;

template <typename T>
bool assign (T & result, std::string && data);

template <>
inline bool assign<std::int64_t> (std::int64_t & result, std::string && data)
{
    if (data.size() > sizeof(std::int64_t))
        return false;

    fixed_packer<std::int64_t> p;
    std::memset(p.bytes, 0, sizeof(p.bytes));
    std::memcpy(p.bytes, data.data(), data.size());
    result = p.value;
    return true;
}

template <>
inline bool assign<double> (double & result, std::string && data)
{
    if (data.size() != sizeof(double))
        return false;

    fixed_packer<double> p;
    std::memcpy(p.bytes, data.data(), data.size());

    if (std::isnan(p.value))
        return false;

    result = p.value;
    return true;
}

template <>
inline bool assign<std::string> (std::string & result, std::string && data)
{
    result = std::move(data);
    return true;
}

template <>
class keyvalue_database_t::impl
{
    static constexpr char const * CFNAME = "debby";

private:
    ::rocksdb::DB * _dbh {nullptr};
    std::vector<::rocksdb::ColumnFamilyHandle *> _handles;
    fs::path _path;

private:
    static ::rocksdb::ColumnFamilyHandle * create_column_family (::rocksdb::DB * dbh
        , fs::path const & path, error * perr)
    {
        ::rocksdb::ColumnFamilyHandle * cf = nullptr;
        auto status = dbh->CreateColumnFamily(::rocksdb::ColumnFamilyOptions(), CFNAME, & cf);

        if (!status.ok()) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , fmt::format("{}: {}", pfs::utf8_encode_path(path), status.ToString()));
            return nullptr;
        }

        return cf;
    }

public:
    impl () = default;

    impl (impl && other) noexcept
    {
        std::swap(_dbh, other._dbh);
        _handles = std::move(other._handles);
        _path = std::move(other._path);
    }

    impl (fs::path const & path, rocksdb::options_type opts, bool create_if_missing, error * perr)
    {
        ::rocksdb::DB * dbh = nullptr;

        ::rocksdb::Options o;

        // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
        if (opts.optimize) {
            o.IncreaseParallelism();
            o.OptimizeLevelStyleCompaction();
        }

        // Use this if your DB is very small (like under 1GB) and you don't want to
        // spend lots of memory for memtables.
        if (opts.small_db)
            o.OptimizeForSmallDb();

        // If true, the database will be created if it is missing.
        // Default: false
        o.create_if_missing = create_if_missing;

        // Aggressively check consistency of the data.
        o.paranoid_checks = true;

        // Maximal info log files to be kept.
        // Default: 1000
        o.keep_log_file_num = opts.keep_log_file_num;

        // If true, missing column families will be automatically created.
        // Default: false
        o.create_missing_column_families = true;

        if (fs::exists(path))
            o.create_if_missing = false;

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
        if (!fs::exists(path) && !o.create_if_missing) {
            pfs::throw_or(perr, make_error_code(errc::database_not_found), pfs::utf8_encode_path(path));
            return;
        }

        // Open DB.
        // `path` is the path to a directory containing multiple database files
        //
        // ATTENTION! ROCKSDB_LITE causes a segmentaion fault while open the database
        //::rocksdb::Status status = ::rocksdb::DB::Open(o, fs::utf8_encode(path), & dbh);

        std::vector<::rocksdb::ColumnFamilyHandle *> handles;
        std::vector<::rocksdb::ColumnFamilyDescriptor> column_families;
        column_families.emplace_back(::rocksdb::kDefaultColumnFamilyName, ::rocksdb::ColumnFamilyOptions());
        column_families.emplace_back(CFNAME, ::rocksdb::ColumnFamilyOptions());
        auto status = ::rocksdb::DB::Open(o, pfs::utf8_encode_path(path), column_families, & handles, & dbh);

        if (!status.ok()) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , fmt::format("{}: {}", pfs::utf8_encode_path(path), status.ToString()));
            return;
        }

        _dbh = dbh;
        _path = path;
        _handles = std::move(handles);
    }

    impl (fs::path const & path, bool create_if_missing, error * perr)
        : impl(path, rocksdb::options_type{}, create_if_missing, perr)
    {}

    impl & operator = (impl && other) noexcept
    {
        this->~impl();
        std::swap(_dbh, other._dbh);
        _handles = std::move(other._handles);
        _path = std::move(other._path);
        return *this;
    }

    ~impl ()
    {
        if (_dbh != nullptr) {
            // _dbh->FlushWAL(true  /* sync */);
            // ::rocksdb::FlushOptions options;
            // options.wait = true;
            //_dbh->CancelAllBackgroundWork(true  /* wait */);

            for (auto h: _handles) {
                if (h != nullptr)
                    _dbh->DestroyColumnFamilyHandle(h);
            }

            _handles.clear();

            _dbh->Close();
            delete _dbh;
            _dbh = nullptr;
        }

        _path.clear();
    }

public:
    /**
    * Removes value for @a key.
    */
    void remove (keyvalue_database_t::key_type const & key, error * perr)
    {
        if (_dbh == nullptr)
            return;

        if (_handles[1] == nullptr)
            return;

        // Remove the database entry (if any) for "key". Returns OK on success,
        // and a non-OK status on error. It is not an error if "key" did not exist
        // in the database.
        // Note: consider setting options.sync = true.
        ::rocksdb::WriteOptions write_opts;
        write_opts.sync = true;
        ::rocksdb::Status status = _dbh->Delete(write_opts, _handles[1], key);

        if (!status.ok()) {
            if (!status.IsNotFound()) {
                pfs::throw_or(perr, make_error_code(errc::backend_error)
                    , tr::f_("remove failure for key: {}: {}", key, status.ToString()));

                return;
            }
        }
    }

   void clear (error * perr = nullptr)
   {
        if (_dbh == nullptr)
            return;

        if (_handles[1] == nullptr)
            return;

        auto status = _dbh->DropColumnFamily(_handles[1]);

        if (!status.ok()) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("clear failure (drop column family): {}", status.ToString()));
            return;
        }

        status = _dbh->DestroyColumnFamilyHandle(_handles[1]);

        if (!status.ok()) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("clear failure (destroy column family): {}", status.ToString()));
            return;
        }

        _handles[1] = create_column_family(_dbh, _path, perr);
   }

    /**
    * Writes @c data into database by @a key.
    *
    * @details Attempt to write @c nullptr data interpreted as delete operation
    *          for key.
    *
    * @return @c true on success, @c false otherwise.
    */
    bool put (keyvalue_database_t::key_type const & key, char const * data, std::size_t size, error * perr)
    {
        if (_dbh == nullptr)
            return false;

        if (_handles[1] == nullptr)
            return false;

        // Attempt to write `null` data interpreted as delete operation for key
        if (!data) {
            error err;
            remove(key, & err);

            if (err) {
                pfs::throw_or(perr, std::move(err));
                return false;
            }

            return true;
        }

        auto status = _dbh->Put(::rocksdb::WriteOptions(), _handles[1], key, ::rocksdb::Slice(data, size));

        if (!status.ok()) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("write failure for key: {}: {}", key, status.ToString()));
            return false;
        }

        return true;
    }

    template <typename T>
    T get (std::string const & key, error * perr)
    {
        PFS__TERMINATE(_dbh != nullptr, "");
        PFS__TERMINATE(_handles[1] != nullptr, "");

        T result;

        std::string buf;
        ::rocksdb::Status status = _dbh->Get(::rocksdb::ReadOptions(), _handles[1], key, & buf);

        if (status.ok()) {
            if (!assign<T>(result, std::move(buf))) {
                pfs::throw_or(perr, make_unsuitable_error(key));
                return T{};
            }
        } else {
            pfs::throw_or(perr, make_error_code(
                status.IsNotFound()
                    ? errc::key_not_found
                    : errc::backend_error)
                , status.IsNotFound()
                    ? tr::f_("key not found: {}", key)
                    : tr::f_("read failure for key: {}: {}", key, status.ToString()));

            return T{};
        }

        return result;
    }
};

constexpr char const * keyvalue_database<backend_enum::rocksdb>::impl::CFNAME;

template keyvalue_database<backend_enum::rocksdb>::keyvalue_database (impl && d);
template keyvalue_database<backend_enum::rocksdb>::keyvalue_database (keyvalue_database && other) noexcept;
template keyvalue_database<backend_enum::rocksdb>::~keyvalue_database ();
template keyvalue_database<backend_enum::rocksdb> & keyvalue_database<backend_enum::rocksdb>::operator = (keyvalue_database && other) noexcept;

template <>
void keyvalue_database_t::clear (error * perr)
{
    if (_d != nullptr)
        _d->clear(perr);
}

namespace rocksdb {

keyvalue_database_t
make_kv (fs::path const & path, options_type opts, bool create_if_missing, error * perr)
{
    return keyvalue_database_t{keyvalue_database_t::impl{path, std::move(opts), create_if_missing, perr}};
}

keyvalue_database_t
make_kv (fs::path const & path, bool create_if_missing, error * perr)
{
    return keyvalue_database_t{keyvalue_database_t::impl{path, create_if_missing, perr}};
}

bool wipe (fs::path const & path, error * perr)
{
    std::error_code ec;

    if (path.empty())
        return false;

    if (path == "/")
        return false;

    if (fs::exists(path, ec) && fs::is_directory(path, ec))
        fs::remove_all(path, ec);

    if (ec) {
        pfs::throw_or(perr, ec, tr::f_("wipe RocksDB database: {}", pfs::utf8_encode_path(path)));
        return false;
    }

    return true;
}

} // namespace rocksdb

template <>
void keyvalue_database_t::set_arithmetic (key_type const & key, std::int64_t value, std::size_t size, error * perr)
{
    if (_d != nullptr) {
        char buf[sizeof(fixed_packer<std::int64_t>)];
        auto p = new (buf) fixed_packer<std::int64_t>{};
        p->value = value;
        _d->put(key, buf, size, perr);
    }
}

template <>
void keyvalue_database_t::set_arithmetic (key_type const & key, double value, std::size_t /*size*/, error * perr)
{
    if (_d != nullptr) {
        char buf[sizeof(fixed_packer<double>)];
        auto p = new (buf) fixed_packer<double>{};
        p->value = value;
        _d->put(key, buf, sizeof(fixed_packer<double>), perr);
    }
}

template <>
void keyvalue_database_t::set_chars (key_type const & key, char const * data, std::size_t size, error * perr)
{
    if (_d != nullptr)
        _d->put(key, data, size, perr);
}

template <>
void
keyvalue_database_t::remove (key_type const & key, error * perr)
{
    if (_d != nullptr)
        _d->remove(key, perr);
}

template <>
std::int64_t keyvalue_database_t::get_int64 (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->get<std::int64_t>(key, perr);
}

template <>
double keyvalue_database_t::get_double (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->get<double>(key, perr);
}

template <>
std::string keyvalue_database_t::get_string (key_type const & key, error * perr)
{
    PFS__TERMINATE(_d != nullptr, "");
    return _d->get<std::string>(key, perr);
}

DEBBY__NAMESPACE_END

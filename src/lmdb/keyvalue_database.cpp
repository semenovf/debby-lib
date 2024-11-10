////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.07.13 Initial version.
//      2024.11.04 V2 started.
////////////////////////////////////////////////////////////////////////////////
#include "../kv_common.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "debby/lmdb.hpp"
#include "lib/lmdb.h"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <algorithm>

namespace fs = pfs::filesystem;

DEBBY__NAMESPACE_BEGIN

// LMDB errors range from -30800 to -30999. So the custom error will start at -10000
constexpr int UNSUITABLE_VALUE_ERROR = -10001;

using keyvalue_database_t = keyvalue_database<backend_enum::lmdb>;

template <typename T>
struct mv_data_caster;

template <typename T>
struct mv_data_caster<T const *>
{
    static decltype(MDB_val::mv_data) cast (T const * p)
    {
        return static_cast<decltype(MDB_val::mv_data)>(const_cast<T *>(p));
    }
};

template <typename T>
decltype(MDB_val::mv_data) mv_data_cast (T p)
{
    return mv_data_caster<T>::cast(p);
}

template <typename T>
bool assign (T & result, MDB_val const & val);

template <>
inline bool assign<std::int64_t> (std::int64_t & result, MDB_val const & val)
{
    if (val.mv_size > sizeof(std::int64_t))
        return false;

    fixed_packer<std::int64_t> p;
    std::memset(p.bytes, 0, sizeof(p.bytes));
    std::memcpy(p.bytes/* + (sizeof(std::int64_t) - val.mv_size)*/, val.mv_data, val.mv_size);
    result = p.value;
    return true;
}

template <>
inline bool assign<double> (double & result, MDB_val const & val)
{
    if (val.mv_size != sizeof(double))
        return false;

    fixed_packer<double> p;
    std::memcpy(p.bytes, val.mv_data, val.mv_size);

    if (std::isnan(p.value))
        return false;

    result = p.value;
    return true;
}

template <>
inline bool assign<std::string> (std::string & result, MDB_val const & val)
{
    result = std::string(static_cast< char const *>(val.mv_data), val.mv_size);
    return true;
}

template <>
class keyvalue_database_t::impl
{
private:
    MDB_env * _env {nullptr};
    MDB_dbi _dbh {0};
    fs::path _path;

public:
    impl () = default;

    impl (impl && other) noexcept
    {
        std::swap(_env, other._env);
        std::swap(_dbh, other._dbh);
        _path = std::move(other._path);
    }

    impl (fs::path const & path, lmdb::options_type opts, bool create_if_missing, error * perr)
    {
        MDB_env * env = nullptr;
        MDB_dbi dbh = 0;

        if (opts.env == 0 && opts.db == 0) {
            opts = lmdb::options_type {
                  MDB_NOSUBDIR // No environment directory
                , MDB_CREATE   // Create DB if not already existing
            };
        }

        if (create_if_missing)
            opts.db |= MDB_CREATE;
        else
            opts.db &= ~MDB_CREATE;

        MDB_txn * txn = nullptr;
        int rc = MDB_SUCCESS;

        do {
            rc = mdb_env_create(& env);

            if (rc != MDB_SUCCESS)
                break;

            //rc = mdb_env_set_maxdbs(rep.env, sizeof(DB_NAMES) / sizeof(DB_NAMES[0]));
            //if (rc != MDB_SUCCESS)
            //    break;

            rc = mdb_env_open(env, fs::utf8_encode(path).c_str(), static_cast<unsigned int>(opts.env), 0600);

            if (rc != MDB_SUCCESS)
                break;

            rc = mdb_txn_begin(env, nullptr, 0, & txn);

            if (rc != MDB_SUCCESS)
                break;

            rc = mdb_dbi_open(txn, nullptr, static_cast<unsigned int>(opts.db), & dbh);

            if (rc != MDB_SUCCESS)
                break;

            rc = mdb_txn_commit(txn);

            if (rc != MDB_SUCCESS)
                break;

            txn = nullptr;
        } while (false);

        if (rc != MDB_SUCCESS) {
            if (txn)
                mdb_txn_abort(txn);

            if (dbh) {
                mdb_dbi_close(env, dbh);
                dbh = 0;
            }

            if (env != nullptr) {
                mdb_env_close(env);
                env = nullptr;
            }

            pfs::throw_or(perr, error {errc::backend_error, fs::utf8_encode(path), mdb_strerror(rc)});

            return;
        }

        _env = env;
        _dbh = dbh;
        _path = path;
    }

    impl (fs::path const & path, bool create_if_missing, error * perr)
        : impl(path, lmdb::options_type{}, create_if_missing, perr)
    {}

    impl & operator = (impl && other) noexcept
    {
        this->~impl();
        std::swap(_env, other._env);
        std::swap(_dbh, other._dbh);
        _path = std::move(other._path);
        return *this;
    }

    ~impl ()
    {
        if (_env != nullptr) {
            if (_dbh) {
                mdb_dbi_close(_env, _dbh);
                _dbh = 0;
            }

            mdb_env_close(_env);
            _env = nullptr;
        }

        _path.clear();
    }

private:
    template <typename F>
    int perform_transaction (F && f, unsigned int flags)
    {
        MDB_txn * txn = nullptr;
        int rc = MDB_SUCCESS;

        do {
            rc = mdb_txn_begin(_env, nullptr, flags, & txn);

            if (rc != MDB_SUCCESS)
                break;

            rc = f(txn);

            if (rc != MDB_SUCCESS)
                break;

            rc = mdb_txn_commit(txn);

            if (rc != MDB_SUCCESS)
                break;

            txn = nullptr;
        } while (false);

        if (rc != MDB_SUCCESS) {
            if (txn)
                mdb_txn_abort(txn);
        }

        return rc;
    }

public:
    /**
     * Removes value for @a key.
     */
    void remove (keyvalue_database_t::key_type const & key, error * perr)
    {
        auto rc = perform_transaction([this, & key] (MDB_txn * txn) -> int {
            MDB_val k;
            k.mv_data = mv_data_cast(key.c_str());
            k.mv_size = key.size();

            return mdb_del(txn, _dbh, & k, nullptr);
        }, 0);

        if (rc != MDB_SUCCESS) {
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::f_("remove failure for key: '{}'", key)
                , mdb_strerror(rc)
            });

            return;
        }
    }

    void clear (error * perr = nullptr)
    {
        auto rc = perform_transaction([this] (MDB_txn * txn) -> int {
            return mdb_drop(txn, _dbh, 0);
        }, 0);

        if (rc != MDB_SUCCESS) {
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::_("LMDB database cleaning failure")
                , mdb_strerror(rc)
            });
        }
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
        if (_dbh == 0)
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

        auto rc = perform_transaction([this, & key, & data, & size] (MDB_txn * txn) -> int {
            MDB_val ky;
            ky.mv_data = mv_data_cast(key.c_str());
            ky.mv_size  = key.size();

            MDB_val val;
            val.mv_data = mv_data_cast(data);
            val.mv_size = size;

            return mdb_put(txn, _dbh, & ky, & val, 0);
        }, 0);

        if (rc != MDB_SUCCESS) {
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::f_("write failure for key: '{}'", key)
                , mdb_strerror(rc)
            });

            return false;
        }

        return true;
    }

    template <typename T>
    T get (std::string const & key, error * perr)
    {
        T result;

        auto rc = perform_transaction([this, & key, & result] (MDB_txn * txn) -> int {
            MDB_val k;
            MDB_val val;
            k.mv_data = mv_data_cast(key.c_str());
            k.mv_size  = key.size();

            auto rc = mdb_get(txn, _dbh, & k, & val);

            if (rc == MDB_SUCCESS) {
                if (!assign<T>(result, val))
                    rc = UNSUITABLE_VALUE_ERROR;
            }

            return rc;
        }, MDB_RDONLY);

        if (rc != MDB_SUCCESS) {
            if (rc == UNSUITABLE_VALUE_ERROR) {
                pfs::throw_or(perr, make_unsuitable_error(key));
            } else {
                error err {
                    rc == MDB_NOTFOUND
                        ? errc::key_not_found
                        : errc::backend_error
                    , rc == MDB_NOTFOUND
                        ? tr::f_("key not found: '{}'", key)
                        : tr::f_("read failure for key: '{}'", key)
                    , rc == MDB_NOTFOUND
                        ? ""
                        : mdb_strerror(rc)
                };

                pfs::throw_or(perr, std::move(err));
            };

            return T{};
        }

        return result;
    }
};

//template keyvalue_database<backend_enum::lmdb>::keyvalue_database ();
template keyvalue_database<backend_enum::lmdb>::keyvalue_database (impl && d);
template keyvalue_database<backend_enum::lmdb>::keyvalue_database (keyvalue_database && other) noexcept;
template keyvalue_database<backend_enum::lmdb>::~keyvalue_database ();
template keyvalue_database<backend_enum::lmdb> & keyvalue_database<backend_enum::lmdb>::operator = (keyvalue_database && other) noexcept;

template <>
void keyvalue_database_t::clear (error * perr)
{
    if (_d != nullptr)
        _d->clear(perr);
}

namespace lmdb {

keyvalue_database_t
make_kv (fs::path const & path, options_type opts, bool create_if_missing, error * perr)
{
    return keyvalue_database_t{keyvalue_database_t::impl{path, opts, create_if_missing, perr}};
}

keyvalue_database_t
make_kv (fs::path const & path, bool create_if_missing, error * perr)
{
    return keyvalue_database_t{keyvalue_database_t::impl{path, create_if_missing, perr}};
}

bool wipe (fs::path const & path, error * perr)
{
    std::error_code ec1;
    std::error_code ec2;

    auto lck_path {path};
    lck_path += PFS__LITERAL_PATH("-lock");

    if (fs::exists(path, ec1) && fs::is_regular_file(path, ec1))
        fs::remove(path, ec1);

    if (fs::exists(lck_path, ec2) && fs::is_regular_file(lck_path, ec2))
        fs::remove(lck_path, ec2);

    if (ec1 || ec2) {
        if (ec1)
            pfs::throw_or(perr, ec1, tr::_("wipe LMDB database failure"), fs::utf8_encode(path));
        else if (ec2)
            pfs::throw_or(perr, ec2, tr::_("wipe LMDB database failure"), fs::utf8_encode(lck_path));

        return false;
    }

    return true;
}

} // namespace lmdb

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

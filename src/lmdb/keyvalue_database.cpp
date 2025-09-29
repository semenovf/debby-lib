////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.07.13 Initial version.
//      2024.11.04 V2 started.
////////////////////////////////////////////////////////////////////////////////
#include "../keyvalue_database_common.hpp"
#include "debby/keyvalue_database.hpp"
#include "debby/lmdb.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>
#include <lmdb.h>
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
std::enable_if_t<std::is_arithmetic<T>::value, bool>
assign (T & result, MDB_val const & val)
{
    fixed_packer<T> p;
    std::memset(p.bytes, 0, sizeof(p.bytes));
    std::memcpy(p.bytes, val.mv_data, val.mv_size);

    if (std::isnan(p.value))
        return false;

    result = p.value;
    return true;
}

template <typename T>
inline std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value, bool>
assign (std::string & result, MDB_val const & val)
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

            rc = mdb_env_open(env, pfs::utf8_encode_path(path).c_str()
                , static_cast<unsigned int>(opts.env), 0600);

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

            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , fmt::format("{}: {}", pfs::utf8_encode_path(path), mdb_strerror(rc)));

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
    void clear (error * perr = nullptr)
    {
        auto rc = perform_transaction([this] (MDB_txn * txn) -> int {
            return mdb_drop(txn, _dbh, 0);
        }, 0);

        if (rc != MDB_SUCCESS) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("LMDB database cleaning failure: {}", mdb_strerror(rc)));
        }
    }

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
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("remove failure for key: {}: {}", key, mdb_strerror(rc)));
            return;
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
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("write failure for key: {}: {}", key, mdb_strerror(rc)));
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
                        ? make_error_code(errc::key_not_found)
                        : make_error_code(errc::backend_error)
                    , rc == MDB_NOTFOUND
                        ? tr::f_("key not found: {}", key)
                        : tr::f_("read failure for key: {}: {}", key, mdb_strerror(rc))
                };

                pfs::throw_or(perr, std::move(err));
            };

            return T{};
        }

        return result;
    }
};

template keyvalue_database_t::keyvalue_database ();
template keyvalue_database_t::keyvalue_database (impl && d) noexcept;
template keyvalue_database_t::keyvalue_database (keyvalue_database && other) noexcept;
template keyvalue_database_t::~keyvalue_database ();
template keyvalue_database_t & keyvalue_database_t::operator = (keyvalue_database && other) noexcept;

template <>
void keyvalue_database_t::clear (error * perr)
{
    _d->clear(perr);
}

template <>
void keyvalue_database_t::remove (key_type const & key, error * perr)
{
    _d->remove(key, perr);
}

template <>
void keyvalue_database_t::set (key_type const & key, char const * value, std::size_t len
    , error * perr)
{
    _d->put(key, value, len, perr);
}

template <>
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value, void>
keyvalue_database_t::set (key_type const & key, T value, error * perr)
{
    char buf[sizeof(fixed_packer<T>)];
    auto p = new (buf) fixed_packer<T>{};
    p->value = value;
    _d->put(key, buf, sizeof(T), perr);
}

template <>
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value || std::is_same<std::decay_t<T>, std::string>::value, std::decay_t<T>>
keyvalue_database_t::get (key_type const & key, error * perr)
{
    return _d->template get<std::decay_t<T>>(key, perr);
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
        if (ec1) {
            pfs::throw_or(perr, ec1, tr::f_("wipe LMDB database failure: {}"
                , pfs::utf8_encode_path(path)));
        } else if (ec2) {
            pfs::throw_or(perr, ec2, tr::f_("wipe LMDB database failure: {}"
                , pfs::utf8_encode_path(lck_path)));
        }

        return false;
    }

    return true;
}

} // namespace lmdb

#define DEBBY__LMDB_SET(t) \
    template void keyvalue_database_t::set<t> (key_type const & key, t value, error * perr);

#define DEBBY__LMDB_GET(t) \
    template t keyvalue_database_t::get<t> (key_type const & key, error * perr);

DEBBY__LMDB_SET(bool)
DEBBY__LMDB_SET(char)
DEBBY__LMDB_SET(signed char)
DEBBY__LMDB_SET(unsigned char)
DEBBY__LMDB_SET(short int)
DEBBY__LMDB_SET(unsigned short int)
DEBBY__LMDB_SET(int)
DEBBY__LMDB_SET(unsigned int)
DEBBY__LMDB_SET(long int)
DEBBY__LMDB_SET(unsigned long int)
DEBBY__LMDB_SET(long long int)
DEBBY__LMDB_SET(unsigned long long int)
DEBBY__LMDB_SET(float)
DEBBY__LMDB_SET(double)

DEBBY__LMDB_GET(bool)
DEBBY__LMDB_GET(char)
DEBBY__LMDB_GET(signed char)
DEBBY__LMDB_GET(unsigned char)
DEBBY__LMDB_GET(short int)
DEBBY__LMDB_GET(unsigned short int)
DEBBY__LMDB_GET(int)
DEBBY__LMDB_GET(unsigned int)
DEBBY__LMDB_GET(long int)
DEBBY__LMDB_GET(unsigned long int)
DEBBY__LMDB_GET(long long int)
DEBBY__LMDB_GET(unsigned long long int)
DEBBY__LMDB_GET(float)
DEBBY__LMDB_GET(double)
DEBBY__LMDB_GET(std::string)

DEBBY__NAMESPACE_END

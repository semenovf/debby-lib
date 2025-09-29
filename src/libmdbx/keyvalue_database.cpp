////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.06 Initial version.
//      2024.11.10 V2 started.
//      2025.09.29 Changed set/get implementation.
////////////////////////////////////////////////////////////////////////////////
#include "../keyvalue_database_common.hpp"
#include "debby/keyvalue_database.hpp"
#include "debby/mdbx.hpp"
#include <pfs/filesystem.hpp>
#include <pfs/i18n.hpp>
#include <mdbx.h>
#include <utility>

namespace fs = pfs::filesystem;

DEBBY__NAMESPACE_BEGIN

constexpr int UNSUITABLE_VALUE_ERROR = -10001;

using keyvalue_database_t = keyvalue_database<backend_enum::mdbx>;

template <typename T>
struct iov_base_caster;

template <typename T>
struct iov_base_caster<T const *>
{
    static decltype(MDBX_val::iov_base) cast (T const * p)
    {
        return static_cast<decltype(MDBX_val::iov_base)>(const_cast<T *>(p));
    }
};

template <typename T>
decltype(MDBX_val::iov_base) iov_base_cast (T p)
{
    return iov_base_caster<T>::cast(p);
}

// template <typename T>
// static bool assign (T & result, MDBX_val const & val);
//
// template <>
// inline bool assign<std::int64_t> (std::int64_t & result, MDBX_val const & val)
// {
//     if (val.iov_len > sizeof(std::int64_t))
//         return false;
//
//     fixed_packer<std::int64_t> p;
//     std::memset(p.bytes, 0, sizeof(p.bytes));
//     std::memcpy(p.bytes/* + (sizeof(std::int64_t) - val.mv_size)*/, val.iov_base, val.iov_len);
//     result = p.value;
//     return true;
// }
//
// template <>
// inline bool assign<double> (double & result, MDBX_val const & val)
// {
//     if (val.iov_len != sizeof(double))
//         return false;
//
//     fixed_packer<double> p;
//     std::memcpy(p.bytes, val.iov_base, val.iov_len);
//
//     if (std::isnan(p.value))
//         return false;
//
//     result = p.value;
//     return true;
// }
//
// template <>
// inline bool assign<std::string> (std::string & result, MDBX_val const & val)
// {
//     result = std::string(static_cast< char const *>(val.iov_base), val.iov_len);
//     return true;
// }
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value, bool>
assign (T & result, MDBX_val const & val)
{
    fixed_packer<T> p;
    std::memset(p.bytes, 0, sizeof(p.bytes));
    std::memcpy(p.bytes, val.iov_base, val.iov_len);

    if (std::isnan(p.value))
        return false;

    result = p.value;
    return true;
}

template <typename T>
inline std::enable_if_t<std::is_same<std::decay_t<T>, std::string>::value, bool>
assign (std::string & result, MDBX_val const & val)
{
    result = std::string(static_cast< char const *>(val.iov_base), val.iov_len);
    return true;
}

template <>
class keyvalue_database_t::impl
{
private:
    MDBX_env * _env {nullptr};
    MDBX_dbi _dbh {0};
    fs::path _path;

public:
    impl () = default;

    impl (impl && other) noexcept
    {
        std::swap(_env, other._env);
        std::swap(_dbh, other._dbh);
        _path = std::move(other._path);
    }

    impl (fs::path const & path, mdbx::options_type opts, bool create_if_missing, error * perr)
    {
        MDBX_env * env = nullptr;
        MDBX_dbi dbh = 0;

        if (opts.env == 0 && opts.db == 0) {
            opts = mdbx::options_type {
                  MDBX_NOSUBDIR    // in a filesystem we have the pair of MDBX-files
                                   // which names derived from given pathname by
                                   // appending predefined suffixes.
                // | MDBX_COALESCE    // will aims to coalesce items while recycling
                //                    // a Garbage Collection (DEPRECTED)
                | MDBX_LIFORECLAIM // MDBX_LIFORECLAIM flag turns on LIFO policy
                                   // for recycling a Garbage Collection items,
                                   // instead of FIFO by default. On systems with
                                   // a disk write-back cache, this can
                                   // significantly increase write performance,
                                   // up to several times in a best case scenario.

                , MDBX_CREATE // Create DB if not already existing.
            };
        }

        if (create_if_missing)
            opts.db |= MDBX_CREATE;
        else
            opts.db &= ~MDBX_CREATE;

        MDBX_txn * txn = nullptr;
        int rc = MDBX_SUCCESS;

        do {
            rc = mdbx_env_create(& env);

            if (rc != MDBX_SUCCESS)
                break;

            //rc = mdbx_env_set_maxdbs(rep.env, sizeof(DB_NAMES) / sizeof(DB_NAMES[0]));
            //if (rc != MDBX_SUCCESS)
            //    break;

            rc = mdbx_env_open(env, pfs::utf8_encode_path(path).c_str(), static_cast<MDBX_env_flags_t>(opts.env), 0600);

            if (rc != MDBX_SUCCESS)
                break;

            rc = mdbx_txn_begin(env, nullptr, MDBX_TXN_READWRITE, & txn);

                if (rc != MDBX_SUCCESS)
                    break;

            rc = mdbx_dbi_open(txn, nullptr, static_cast<MDBX_db_flags_t>(opts.db), & dbh);

            if (rc != MDBX_SUCCESS)
                break;

            rc = mdbx_txn_commit(txn);

            if (rc != MDBX_SUCCESS)
                break;

            txn = nullptr;
        } while (false);

        if (rc != MDBX_SUCCESS) {
            if (txn)
                mdbx_txn_abort(txn);

            if (dbh) {
                mdbx_dbi_close(env, dbh);
                dbh = 0;
            }

            if (env != nullptr) {
                mdbx_env_close(env);
                env = nullptr;
            }

            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , fmt::format("{}: {}", pfs::utf8_encode_path(path), mdbx_strerror(rc)));

            return;
        }

        _env = env;
        _dbh = dbh;
        _path = path;
    }

    impl (fs::path const & path, bool create_if_missing, error * perr)
        : impl(path, mdbx::options_type{}, create_if_missing, perr)
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
                mdbx_dbi_close(_env, _dbh);
                _dbh = 0;
            }

            mdbx_env_close(_env);
            _env = nullptr;
        }

        _path.clear();
    }

private:
    template <typename F>
    int perform_transaction (F && f, MDBX_txn_flags_t flags)
    {
        MDBX_txn * txn = nullptr;
        int rc = MDBX_SUCCESS;

        do {
            rc = mdbx_txn_begin(_env, nullptr, flags, & txn);

            if (rc != MDBX_SUCCESS)
                break;

            rc = f(txn);

            if (rc != MDBX_SUCCESS)
                break;

            rc = mdbx_txn_commit(txn);

            if (rc != MDBX_SUCCESS)
                break;

            txn = nullptr;
        } while (false);

        if (rc != MDBX_SUCCESS) {
            if (txn)
                mdbx_txn_abort(txn);
        }

        return rc;
    }

public:
    void clear (error * perr = nullptr)
    {
        auto rc = perform_transaction([this] (MDBX_txn * txn) -> int {
            return mdbx_drop(txn, _dbh, 0);
        }, MDBX_TXN_READWRITE);

        if (rc != MDBX_SUCCESS) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("MDBX database cleaning failure: {}", mdbx_strerror(rc)));
        }
    }

    /**
    * Removes value for @a key.
    */
    void remove (keyvalue_database_t::key_type const & key, error * perr)
    {
        auto rc = perform_transaction([this, & key] (MDBX_txn * txn) -> int {
            MDBX_val k;
            k.iov_base = iov_base_cast(key.c_str());
            k.iov_len  = key.size();

            return mdbx_del(txn, _dbh, & k, nullptr);
        }, MDBX_TXN_READWRITE);

        if (rc != MDBX_SUCCESS) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("remove failure for key: {}: {}", key, mdbx_strerror(rc)));
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

        auto rc = perform_transaction([this, & key, & data, & size] (MDBX_txn * txn) -> int {
            MDBX_val k;
            k.iov_base = iov_base_cast(key.c_str());
            k.iov_len  = key.size();

            MDBX_val val;
            val.iov_base = iov_base_cast(data);
            val.iov_len  = size;

            return mdbx_put(txn, _dbh, & k, & val, MDBX_UPSERT);
        }, MDBX_TXN_READWRITE);

        if (rc != MDBX_SUCCESS) {
            pfs::throw_or(perr, make_error_code(errc::backend_error)
                , tr::f_("write failure for key: {}: {}", key, mdbx_strerror(rc)));

            return false;
        }

        return true;
    }

    template <typename T>
    T get (std::string const & key, error * perr)
    {
        T result;

        auto rc = perform_transaction([this, & key, & result] (MDBX_txn * txn) -> int {
            MDBX_val k;
            MDBX_val val;
            k.iov_base = iov_base_cast(key.c_str());
            k.iov_len  = key.size();

            auto rc = mdbx_get(txn, _dbh, & k, & val);

            if (rc == MDBX_SUCCESS) {
                if (!assign<T>(result, val))
                    rc = UNSUITABLE_VALUE_ERROR;
            }

            return rc;
        }, MDBX_TXN_RDONLY);

        if (rc != MDBX_SUCCESS) {
            if (rc == UNSUITABLE_VALUE_ERROR) {
                pfs::throw_or(perr, make_unsuitable_error(key));
            } else {
                error err {
                      rc == MDBX_NOTFOUND
                        ? make_error_code(errc::key_not_found)
                        : make_error_code(errc::backend_error)
                    , rc == MDBX_NOTFOUND
                        ? tr::f_("key not found: {}", key)
                        : tr::f_("read failure for key: {}: {}", key, mdbx_strerror(rc))
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

namespace mdbx {

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
    lck_path += PFS__LITERAL_PATH("-lck");

    if (fs::exists(path, ec1) && fs::is_regular_file(path, ec1))
        fs::remove(path, ec1);

    if (fs::exists(lck_path, ec2) && fs::is_regular_file(lck_path, ec2))
        fs::remove(lck_path, ec2);

    if (ec1 || ec2) {
        if (ec1)
            pfs::throw_or(perr, ec1, tr::f_("wipe MDBX database failure: {}", pfs::utf8_encode_path(path)));
        else if (ec2)
            pfs::throw_or(perr, ec2, tr::f_("wipe MDBX database failure: {}", pfs::utf8_encode_path(lck_path)));

        return false;
    }

    return true;
}

} // namespace mdbx

#define DEBBY__MDBX_SET(t) \
    template void keyvalue_database_t::set<t> (key_type const & key, t value, error * perr);

#define DEBBY__MDBX_GET(t) \
    template t keyvalue_database_t::get<t> (key_type const & key, error * perr);

DEBBY__MDBX_SET(bool)
DEBBY__MDBX_SET(char)
DEBBY__MDBX_SET(signed char)
DEBBY__MDBX_SET(unsigned char)
DEBBY__MDBX_SET(short int)
DEBBY__MDBX_SET(unsigned short int)
DEBBY__MDBX_SET(int)
DEBBY__MDBX_SET(unsigned int)
DEBBY__MDBX_SET(long int)
DEBBY__MDBX_SET(unsigned long int)
DEBBY__MDBX_SET(long long int)
DEBBY__MDBX_SET(unsigned long long int)
DEBBY__MDBX_SET(float)
DEBBY__MDBX_SET(double)

DEBBY__MDBX_GET(bool)
DEBBY__MDBX_GET(char)
DEBBY__MDBX_GET(signed char)
DEBBY__MDBX_GET(unsigned char)
DEBBY__MDBX_GET(short int)
DEBBY__MDBX_GET(unsigned short int)
DEBBY__MDBX_GET(int)
DEBBY__MDBX_GET(unsigned int)
DEBBY__MDBX_GET(long int)
DEBBY__MDBX_GET(unsigned long int)
DEBBY__MDBX_GET(long long int)
DEBBY__MDBX_GET(unsigned long long int)
DEBBY__MDBX_GET(float)
DEBBY__MDBX_GET(double)
DEBBY__MDBX_GET(std::string)

DEBBY__NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.06 Initial version.
//      2024.11.10 V2 started.
////////////////////////////////////////////////////////////////////////////////
#include "../kv_common.hpp"
#include "debby/keyvalue_database.hpp"
#include "debby/mdbx.hpp"
#include "lib/mdbx.h"
#include <pfs/filesystem.hpp>
#include <pfs/i18n.hpp>
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

template <typename T>
static bool assign (T & result, MDBX_val const & val);

template <>
inline bool assign<std::int64_t> (std::int64_t & result, MDBX_val const & val)
{
    if (val.iov_len > sizeof(std::int64_t))
        return false;

    fixed_packer<std::int64_t> p;
    std::memset(p.bytes, 0, sizeof(p.bytes));
    std::memcpy(p.bytes/* + (sizeof(std::int64_t) - val.mv_size)*/, val.iov_base, val.iov_len);
    result = p.value;
    return true;
}

template <>
inline bool assign<double> (double & result, MDBX_val const & val)
{
    if (val.iov_len != sizeof(double))
        return false;

    fixed_packer<double> p;
    std::memcpy(p.bytes, val.iov_base, val.iov_len);

    if (std::isnan(p.value))
        return false;

    result = p.value;
    return true;
}

template <>
inline bool assign<std::string> (std::string & result, MDBX_val const & val)
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
                | MDBX_COALESCE    // will aims to coalesce items while recycling
                                   // a Garbage Collection
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

            rc = mdbx_env_open(env, fs::utf8_encode(path).c_str(), static_cast<MDBX_env_flags_t>(opts.env), 0600);

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

            pfs::throw_or(perr, error {errc::backend_error, fs::utf8_encode(path), mdbx_strerror(rc)});

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
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::f_("remove failure for key: '{}'", key)
                , mdbx_strerror(rc)
            });

            return;
        }
    }

    void clear (error * perr = nullptr)
    {
        auto rc = perform_transaction([this] (MDBX_txn * txn) -> int {
            return mdbx_drop(txn, _dbh, 0);
        }, MDBX_TXN_READWRITE);

        if (rc != MDBX_SUCCESS) {
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::_("MDBX database cleaning failure")
                , mdbx_strerror(rc)
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
            pfs::throw_or(perr, error {
                  errc::backend_error
                , tr::f_("write failure for key: '{}'", key)
                , mdbx_strerror(rc)
            });

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
                        ? errc::key_not_found
                        : errc::backend_error
                    , rc == MDBX_NOTFOUND
                        ? tr::f_("key not found: '{}'", key)
                        : tr::f_("read failure for key: '{}'", key)
                    , rc == MDBX_NOTFOUND
                        ? ""
                        : mdbx_strerror(rc)
                };

                pfs::throw_or(perr, std::move(err));
            };

            return T{};
        }

        return result;
    }
};

template keyvalue_database_t::keyvalue_database (impl && d) noexcept;
template keyvalue_database_t::keyvalue_database (keyvalue_database && other) noexcept;
template keyvalue_database_t::~keyvalue_database ();
template keyvalue_database_t & keyvalue_database_t::operator = (keyvalue_database && other) noexcept;

template <>
void keyvalue_database_t::clear (error * perr)
{
    if (_d != nullptr)
        _d->clear(perr);
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
            pfs::throw_or(perr, ec1, tr::_("wipe MDBX database failure"), fs::utf8_encode(path));
        else if (ec2)
            pfs::throw_or(perr, ec2, tr::_("wipe MDBX database failure"), fs::utf8_encode(lck_path));

        return false;
    }

    return true;
}

} // namespace mdbx

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

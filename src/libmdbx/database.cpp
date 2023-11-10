////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/filesystem.hpp"
#include "pfs/i18n.hpp"
#include "pfs/assert.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/backend/libmdbx/database.hpp"
#include "../kv_common.hpp"
#include <mdbx.h>
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace fs = pfs::filesystem;

namespace debby {

namespace backend {
namespace libmdbx {

static_assert(std::is_same<database::native_type, MDBX_dbi>::value
    , "libmdbx::database::native_type must be same type as MDBX_dbi");

static_assert(sizeof(database::options_type::env) == sizeof(std::underlying_type<MDBX_env_flags_t>::type)
    , "libmdbx::database::options_type::env must be same type as MDBX_env_flags_t underlying type");

static_assert(sizeof(database::options_type::db) == sizeof(std::underlying_type<MDBX_db_flags_t>::type)
    , "libmdbx::database::options_type::db must be same type as MDBX_db_flags_t underlying type");

static char const * NULL_HANDLER = tr::noop_("uninitialized database handler");

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

static database::options_type default_options ()
{
    database::options_type options = {
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

    return options;
}

static bool get (database::rep_type const * rep
    , database::key_type const & key, std::string * string_result
    , blob_t * blob_result, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    MDBX_txn * txn = nullptr;
    int rc = MDBX_SUCCESS;

    do {
        rc = mdbx_txn_begin(rep->env, nullptr, MDBX_TXN_RDONLY, & txn);

        if (rc != MDBX_SUCCESS)
            break;

        MDBX_val k;
        k.iov_base = iov_base_cast(key.c_str());
        k.iov_len  = key.size();

        MDBX_val val;

        rc = mdbx_get(txn, rep->dbh, & k, & val);

        if (rc != MDBX_SUCCESS)
            break;

        rc = mdbx_txn_commit(txn);

        if (rc != MDBX_SUCCESS)
            break;

        txn = nullptr;

        if (string_result) {
            *string_result = std::string(static_cast<char const *>(val.iov_base)
                , val.iov_len);
            return true;
        }

        if (blob_result) {
            blob_result->resize(val.iov_len);
            std::memcpy(blob_result->data()
                , static_cast<char const *>(val.iov_base)
                , val.iov_len);
            return true;
        }
    } while (false);

    if (rc != MDBX_SUCCESS) {
        if (txn)
            mdbx_txn_abort(txn);

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

    MDBX_txn * txn = nullptr;
    int rc = MDBX_SUCCESS;

    do {
        rc = mdbx_txn_begin(rep->env, nullptr, MDBX_TXN_READWRITE, & txn);

        if (rc != MDBX_SUCCESS)
            break;

        MDBX_val k;
        k.iov_base = iov_base_cast(key.c_str());
        k.iov_len  = key.size();

        rc = mdbx_del(txn, rep->dbh, & k, nullptr);

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

        error err {
              errc::backend_error
            , tr::f_("remove failure for key: '{}'", key)
            , mdbx_strerror(rc)
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

/**
 * Writes @c data into database by @a key.
 *
 * @details Attempt to write @c nullptr data interpreted as delete operation
 *          for key.
 *
 * @return @c true on success, @c false otherwise.
 */
static bool put (database::rep_type * rep, database::key_type const & key
    , char const * data, std::size_t len, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER);

    // Attempt to write `null` data interpreted as delete operation for key
    if (!data)
        return remove(rep, key, perr);

    MDBX_txn * txn = nullptr;
    int rc = MDBX_SUCCESS;

    do {
        rc = mdbx_txn_begin(rep->env, nullptr, MDBX_TXN_READWRITE, & txn);

        if (rc != MDBX_SUCCESS)
            break;

        MDBX_val ky;
        ky.iov_base = iov_base_cast(key.c_str());
        ky.iov_len  = key.size();

        MDBX_val val;
        val.iov_base = iov_base_cast(data);
        val.iov_len  = len;

        rc = mdbx_put(txn, rep->dbh, & ky, & val, MDBX_UPSERT);

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

        error err {
              errc::backend_error
            , tr::f_("write failure for key: '{}'", key)
            , mdbx_strerror(rc)
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
database::make_kv (pfs::filesystem::path const & path, options_type opts, error * perr)
{
    rep_type rep;
    rep.dbh = 0;

    if (opts.env == 0 && opts.db == 0)
        opts = default_options();

    MDBX_txn * txn = nullptr;
    int rc = MDBX_SUCCESS;

    do {
        rc = mdbx_env_create(& rep.env);

        if (rc != MDBX_SUCCESS)
            break;

        //rc = mdbx_env_set_maxdbs(rep.env, sizeof(DB_NAMES) / sizeof(DB_NAMES[0]));
        //if (rc != MDBX_SUCCESS)
        //    break;

        rc = mdbx_env_open(rep.env, fs::utf8_encode(path).c_str()
            , static_cast<MDBX_env_flags_t>(opts.env), 0600);

        if (rc != MDBX_SUCCESS)
            break;

        rc = mdbx_txn_begin(rep.env, nullptr, MDBX_TXN_READWRITE, & txn);

        if (rc != MDBX_SUCCESS)
            break;

        rc = mdbx_dbi_open(txn, nullptr, static_cast<MDBX_db_flags_t>(opts.db)
            , & rep.dbh);

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

        if (rep.dbh) {
            mdbx_dbi_close(rep.env, rep.dbh);
            rep.dbh = 0;
        }

        if (rep.env)
            mdbx_env_close(rep.env);

        error err {
              errc::backend_error
            , fs::utf8_encode(path)
            , mdbx_strerror(rc)
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
database::make_kv (pfs::filesystem::path const & path
    , bool create_if_missing, error * perr)
{
    auto opts = default_options();

    if (create_if_missing)
        opts.db |= MDBX_CREATE;
    else
        opts.db &= ~MDBX_CREATE;

    return make_kv(path, opts, perr);
}

bool database::wipe (fs::path const & path, error * perr)
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
            pfs::throw_or(perr, ec1, tr::_("wipe MDBX database"), fs::utf8_encode(path));
        else if (ec2)
            pfs::throw_or(perr, ec2, tr::_("wipe MDBX database"), fs::utf8_encode(lck_path));

        return false;
    }

    return true;
}

}} // namespace backend::libmdbx

#define BACKEND backend::libmdbx::database

template <>
keyvalue_database<BACKEND>::~keyvalue_database ()
{
    if (_rep.env) {
        if (_rep.dbh) {
            mdbx_dbi_close(_rep.env, _rep.dbh);
            _rep.dbh = 0;
        }

        mdbx_env_close(_rep.env);
        _rep.env = nullptr;
    }

    _rep.path.clear();
}

template <>
keyvalue_database<BACKEND>::keyvalue_database (rep_type && rep)
    : _rep(std::move(rep))
{
    rep.env = nullptr;
    rep.dbh = 0;
}

template <>
keyvalue_database<BACKEND>::keyvalue_database (keyvalue_database && other)
    : keyvalue_database(std::move(other._rep))
{}

template <>
keyvalue_database<BACKEND>::operator bool () const noexcept
{
    return _rep.dbh != 0;
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, std::intmax_t value
    , std::size_t size, error * perr)
{
    char buf[sizeof(std::intmax_t)];
    backend::pack_arithmetic(buf, value, size);
    backend::libmdbx::put(& _rep, key, buf, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, double value
    , std::size_t size, error * perr)
{
    backend::fixed_packer<double> p;
    p.value = value;
    backend::libmdbx::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, float value
    , std::size_t size, error * perr)
{
    backend::fixed_packer<float> p;
    p.value = value;
    backend::libmdbx::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_chars (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    backend::libmdbx::put(& _rep, key, data, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_blob (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    backend::libmdbx::put(& _rep, key, data, size, perr);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::remove
////////////////////////////////////////////////////////////////////////////////
template <>
void
keyvalue_database<BACKEND>::remove (key_type const & key, error * perr)
{
    backend::libmdbx::remove(& _rep, key, perr);
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
    backend::libmdbx::get(& _rep, key, nullptr, & blob, & err);

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
    backend::libmdbx::get(& _rep, key, nullptr, & blob, & err);

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
    backend::libmdbx::get(& _rep, key, nullptr, & blob, & err);

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
    backend::libmdbx::get(& _rep, key, & s, nullptr, perr);
    return s;
}

template <>
blob_t keyvalue_database<BACKEND>::get_blob (key_type const & key, error * perr) const
{
    blob_t blob;
    backend::libmdbx::get(& _rep, key, nullptr, & blob, perr);
    return blob;
}

} // namespace debby

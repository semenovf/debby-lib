////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.07.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/filesystem.hpp"
#include "pfs/i18n.hpp"
#include "pfs/assert.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/backend/lmdb/database.hpp"
#include "../kv_common.hpp"
#include "lib/lmdb.h"
#include <algorithm>
#include <cmath>
#include <type_traits>

namespace fs = pfs::filesystem;

namespace debby {

namespace backend {
namespace lmdb {

static_assert(std::is_same<database::native_type, MDB_dbi>::value
    , "lmdb::database::native_type must be same type as MDBX_dbi");

static char const * NULL_HANDLER_TEXT = tr::noop_("uninitialized database handler");

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

static database::options_type default_options ()
{
    database::options_type options = {
          MDB_NOSUBDIR // No environment directory
        , MDB_CREATE  // Create DB if not already existing
    };

    return options;
}

static bool get (database::rep_type const * rep
    , database::key_type const & key, std::string * string_result
    , blob_t * blob_result, error * perr)
{
    PFS__ASSERT(rep->dbh, NULL_HANDLER_TEXT);

    MDB_txn * txn = nullptr;
    int rc = MDB_SUCCESS;

    do {
        rc = mdb_txn_begin(rep->env, nullptr, 0, & txn);

        if (rc != MDB_SUCCESS)
            break;

        MDB_val k;
        k.mv_data = mv_data_cast(key.c_str());
        k.mv_size  = key.size();

        MDB_val val;

        rc = mdb_get(txn, rep->dbh, & k, & val);

        if (rc != MDB_SUCCESS)
            break;

        rc = mdb_txn_commit(txn);

        if (rc != MDB_SUCCESS)
            break;

        txn = nullptr;

        if (string_result) {
            *string_result = std::string(static_cast<char const *>(val.mv_data)
               , val.mv_size);
            return true;
        }

        if (blob_result) {
            blob_result->resize(val.mv_size);
            std::memcpy(blob_result->data()
                , static_cast<char const *>(val.mv_data)
                , val.mv_size);
            return true;
        }
    } while (false);

    if (rc != MDB_SUCCESS) {
        if (txn)
            mdb_txn_abort(txn);

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
    PFS__ASSERT(rep->dbh, NULL_HANDLER_TEXT);

    MDB_txn * txn = nullptr;
    int rc = MDB_SUCCESS;

    do {
        rc = mdb_txn_begin(rep->env, nullptr, 0, & txn);

        if (rc != MDB_SUCCESS)
            break;

        MDB_val k;
        k.mv_data = mv_data_cast(key.c_str());
        k.mv_size = key.size();

        rc = mdb_del(txn, rep->dbh, & k, nullptr);

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

        error err {
              errc::backend_error
            , tr::f_("remove failure for key: '{}'", key)
            , mdb_strerror(rc)
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
    PFS__ASSERT(rep->dbh, NULL_HANDLER_TEXT);

    // Attempt to write `null` data interpreted as delete operation for key
    if (!data)
        return remove(rep, key, perr);

    MDB_txn * txn = nullptr;
    int rc = MDB_SUCCESS;

    do {
        rc = mdb_txn_begin(rep->env, nullptr, 0, & txn);

        if (rc != MDB_SUCCESS)
            break;

        MDB_val ky;
        ky.mv_data = mv_data_cast(key.c_str());
        ky.mv_size  = key.size();

        MDB_val val;
        val.mv_data = mv_data_cast(data);
        val.mv_size  = len;

        rc = mdb_put(txn, rep->dbh, & ky, & val, 0);

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

        error err {
              errc::backend_error
            , tr::f_("write failure for key: '{}'", key)
            , mdb_strerror(rc)
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

    MDB_txn * txn = nullptr;
    int rc = MDB_SUCCESS;

    do {
        rc = mdb_env_create(& rep.env);

        if (rc != MDB_SUCCESS)
            break;

        //rc = mdb_env_set_maxdbs(rep.env, sizeof(DB_NAMES) / sizeof(DB_NAMES[0]));
        //if (rc != MDB_SUCCESS)
        //    break;

        rc = mdb_env_open(rep.env, fs::utf8_encode(path).c_str()
            , static_cast<unsigned int>(opts.env), 0600);

        if (rc != MDB_SUCCESS)
            break;

        rc = mdb_txn_begin(rep.env, nullptr, 0, & txn);

        if (rc != MDB_SUCCESS)
            break;

        rc = mdb_dbi_open(txn, nullptr, static_cast<unsigned int>(opts.db)
            , & rep.dbh);

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

        if (rep.dbh) {
            mdb_dbi_close(rep.env, rep.dbh);
            rep.dbh = 0;
        }

        if (rep.env)
            mdb_env_close(rep.env);

        error err {
              errc::backend_error
            , fs::utf8_encode(path)
            , mdb_strerror(rc)
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
        opts.db |= MDB_CREATE;
    else
        opts.db &= ~MDB_CREATE;

    return make_kv(path, opts, perr);
}

bool database::wipe (fs::path const & path, error * perr)
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
            pfs::throw_or(perr, ec1, tr::_("wipe LMDB database"), fs::utf8_encode(path));
        else if (ec2)
            pfs::throw_or(perr, ec2, tr::_("wipe LMDB database"), fs::utf8_encode(lck_path));

        return false;
    }

    return true;
}

}} // namespace backend::lmdb

using BACKEND = backend::lmdb::database;

template <>
keyvalue_database<BACKEND>::~keyvalue_database ()
{
    if (_rep.env) {
        if (_rep.dbh) {
            mdb_dbi_close(_rep.env, _rep.dbh);
            _rep.dbh = 0;
        }

        mdb_env_close(_rep.env);
        _rep.env = nullptr;
    }

    _rep.path.clear();
}

template <>
keyvalue_database<BACKEND>::keyvalue_database (rep_type && rep) noexcept
    : _rep(std::move(rep))
{
    rep.env = nullptr;
    rep.dbh = 0;
}

template <>
keyvalue_database<BACKEND>::keyvalue_database (keyvalue_database && other) noexcept
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
    backend::lmdb::put(& _rep, key, buf, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, double value
    , std::size_t size, error * perr)
{
    backend::fixed_packer<double> p;
    p.value = value;
    backend::lmdb::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, float value
    , std::size_t size, error * perr)
{
    backend::fixed_packer<float> p;
    p.value = value;
    backend::lmdb::put(& _rep, key, p.bytes, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_chars (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    backend::lmdb::put(& _rep, key, data, size, perr);
}

template <>
void keyvalue_database<BACKEND>::set_blob (key_type const & key, char const * data
    , std::size_t size, error * perr)
{
    backend::lmdb::put(& _rep, key, data, size, perr);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::remove
////////////////////////////////////////////////////////////////////////////////
template <>
void
keyvalue_database<BACKEND>::remove (key_type const & key, error * perr)
{
    backend::lmdb::remove(& _rep, key, perr);
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
    backend::lmdb::get(& _rep, key, nullptr, & blob, & err);

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
    backend::lmdb::get(& _rep, key, nullptr, & blob, & err);

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
    backend::lmdb::get(& _rep, key, nullptr, & blob, & err);

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
    backend::lmdb::get(& _rep, key, & s, nullptr, perr);
    return s;
}

template <>
blob_t keyvalue_database<BACKEND>::get_blob (key_type const & key, error * perr) const
{
    blob_t blob;
    backend::lmdb::get(& _rep, key, nullptr, & blob, perr);
    return blob;
}

} // namespace debby

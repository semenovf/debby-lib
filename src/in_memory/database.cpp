////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/i18n.hpp"
#include "pfs/memory.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/backend/in_memory/database.hpp"
#include "../kv_common.hpp"

namespace debby {
namespace backend {
namespace in_memory {

database::rep_type
database::make_kv (error * /*perr*/)
{
    return rep_type{pfs::make_unique<std::mutex>(), native_type{}};
}

}} // namespace backend::libmdbx

#define BACKEND backend::in_memory::database

template <>
keyvalue_database<BACKEND>::keyvalue_database (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
keyvalue_database<BACKEND>::keyvalue_database (keyvalue_database && other) = default;

template <>
keyvalue_database<BACKEND>::~keyvalue_database () = default;


template <>
keyvalue_database<BACKEND>::operator bool () const noexcept
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    return true;
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, std::intmax_t value
    , std::size_t /*size*/, error * /*perr*/)
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    _rep.dbh[key] = BACKEND::value_type{value};
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, double value
    , std::size_t /*size*/, error * /*perr*/)
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    _rep.dbh[key] = BACKEND::value_type{value};
}

template <>
void keyvalue_database<BACKEND>::set_arithmetic (key_type const & key, float value
    , std::size_t /*size*/, error * /*perr*/)
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    _rep.dbh[key] = BACKEND::value_type{value};
}

template <>
void keyvalue_database<BACKEND>::set_chars (key_type const & key, char const * data
    , std::size_t size, error * /*perr*/)
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    _rep.dbh[key] = BACKEND::value_type{std::string(data, size)};
}

template <>
void keyvalue_database<BACKEND>::set_blob (key_type const & key, char const * data
    , std::size_t size, error * /*perr*/)
{
    blob_t blob;
    blob.resize(size);
    std::memcpy(blob.data(), data, size);

    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    _rep.dbh[key] = BACKEND::value_type{std::move(blob)};
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::remove
////////////////////////////////////////////////////////////////////////////////
template <>
void
keyvalue_database<BACKEND>::remove (key_type const & key, error * perr)
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    _rep.dbh.erase(key);
}

////////////////////////////////////////////////////////////////////////////////
// keyvalue_database::get
////////////////////////////////////////////////////////////////////////////////

template <>
std::intmax_t
keyvalue_database<BACKEND>::get_integer (key_type const & key, error * perr) const
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    auto pos = _rep.dbh.find(key);

    errc e = errc::success;

    if (pos != _rep.dbh.end()) {
        if (pfs::holds_alternative<bool>(pos->second)) {
            return pfs::get<bool>(pos->second);
        } else if (pfs::holds_alternative<std::intmax_t>(pos->second)) {
            return pfs::get<std::intmax_t>(pos->second);
        } else {
            e = errc::bad_value;
        }
    } else {
        e = errc::key_not_found;
    }

    auto err = (e == errc::bad_value)
        ? backend::make_unsuitable_error(key)
        : backend::make_key_not_found_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return std::intmax_t{0};
}

template <>
float keyvalue_database<BACKEND>::get_float (key_type const & key, error * perr) const
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    auto pos = _rep.dbh.find(key);

    errc e = errc::success;

    if (pos != _rep.dbh.end()) {
        if (pfs::holds_alternative<float>(pos->second)) {
            return pfs::get<float>(pos->second);
        } else if (pfs::holds_alternative<double>(pos->second)) {
            return static_cast<float>(pfs::get<double>(pos->second));
        } else {
            e = errc::bad_value;
        }
    } else {
        e = errc::key_not_found;
    }

    auto err = (e == errc::bad_value)
        ? backend::make_unsuitable_error(key)
        : backend::make_key_not_found_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return float{0};
}

template <>
double keyvalue_database<BACKEND>::get_double (key_type const & key, error * perr) const
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    auto pos = _rep.dbh.find(key);

    errc e = errc::success;

    if (pos != _rep.dbh.end()) {
        if (pfs::holds_alternative<double>(pos->second)) {
            return pfs::get<double>(pos->second);
        } else if (pfs::holds_alternative<float>(pos->second)) {
            return static_cast<double>(pfs::get<float>(pos->second));
        } else {
            e = errc::bad_value;
        }
    } else {
        e = errc::key_not_found;
    }

    auto err = (e == errc::bad_value)
        ? backend::make_unsuitable_error(key)
        : backend::make_key_not_found_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return double{0};
}

template <>
std::string keyvalue_database<BACKEND>::get_string (key_type const & key, error * perr) const
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    auto pos = _rep.dbh.find(key);

    errc e = errc::success;

    if (pos != _rep.dbh.end()) {
        if (pfs::holds_alternative<std::string>(pos->second)) {
            return pfs::get<std::string>(pos->second);
        } else {
            e = errc::bad_value;
        }
    } else {
        e = errc::key_not_found;
    }

    auto err = (e == errc::bad_value)
        ? backend::make_unsuitable_error(key)
        : backend::make_key_not_found_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return std::string{};
}

template <>
blob_t keyvalue_database<BACKEND>::get_blob (key_type const & key, error * perr) const
{
    std::lock_guard<std::mutex> lock{ *_rep.mtx };
    auto pos = _rep.dbh.find(key);

    errc e = errc::success;

    if (pos != _rep.dbh.end()) {
        if (pfs::holds_alternative<blob_t>(pos->second)) {
            return pfs::get<blob_t>(pos->second);
        } else {
            e = errc::bad_value;
        }
    } else {
        e = errc::key_not_found;
    }

    auto err = (e == errc::bad_value)
        ? backend::make_unsuitable_error(key)
        : backend::make_key_not_found_error(key);

    if (perr)
        *perr = err;
    else
        throw err;

    return blob_t{};
}

} // namespace debby


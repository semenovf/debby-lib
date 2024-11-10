////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.07 Initial version.
//      2024.11.04 V2 started.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "debby/error.hpp"
#include "debby/namespace.hpp"
#include "debby/keyvalue_database.hpp"
#include <pfs/i18n.hpp>

DEBBY__NAMESPACE_BEGIN

template <typename T>
union fixed_packer
{
    T value;
    char bytes[sizeof(T)];
};

inline error make_unsuitable_error (std::string const & key)
{
    return error {
          errc::bad_value
        , tr::f_("unsuitable or corrupted data stored"
            " by key: {}, expected double precision floating point", key)
    };
}

inline error make_key_not_found_error (std::string const & key)
{
    return error {
          errc::key_not_found
        , tr::f_("key not found: '{}'", key)
    };
}

template <backend_enum Backend>
keyvalue_database<Backend>::keyvalue_database () = default;

template <backend_enum Backend>
keyvalue_database<Backend>::keyvalue_database (impl && d)
{
    if (_d == nullptr) {
        _d = new impl(std::move(d));
    } else {
        *_d = std::move(d);
    }
}

template <backend_enum Backend>
keyvalue_database<Backend>::keyvalue_database (keyvalue_database && other) noexcept
{
    if (other._d != nullptr) {
        _d = new impl(std::move(*other._d));
    } else {
        delete _d;
        _d = nullptr;
    }
}

template <backend_enum Backend>
keyvalue_database<Backend>::~keyvalue_database ()
{
    if (_d != nullptr)
        delete _d;

    _d = nullptr;
}

template <backend_enum Backend>
keyvalue_database<Backend> & keyvalue_database<Backend>::operator = (keyvalue_database && other) noexcept
{
    if (other._d != nullptr) {
        if (_d != nullptr) {
            *_d = std::move(std::move(*other._d));
        } else {
            _d = new impl(std::move(*other._d));
        }
    } else {
        if (_d != nullptr) {
            delete _d;
            _d = nullptr;
        }
    }

    return *this;
}

DEBBY__NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/debby/error.hpp"
#include "pfs/i18n.hpp"
#include <utility>

namespace debby {
namespace backend {

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

inline void pack_arithmetic (char buf[sizeof(fixed_packer<std::intmax_t>)]
    , std::intmax_t value
    , std::size_t size)
{
    if (size == sizeof(std::intmax_t)) {
        auto p = new (buf) fixed_packer<std::intmax_t>{};
        p->value = value;
    } else {
        switch (size) {
            case sizeof(char): {
                auto p = new (buf) fixed_packer<char>{};
                p->value = static_cast<char>(value);
                break;
            }

            case sizeof(short): {
                auto p = new (buf) fixed_packer<short>{};
                p->value = static_cast<short>(value);
                break;
            }

            case sizeof(int): {
                auto p = new (buf) fixed_packer<int>{};
                p->value = static_cast<int>(value);
                break;
            }

            case sizeof(long int): {
                auto p = new (buf) fixed_packer<long int>{};
                p->value = static_cast<long int>(value);
                break;
            }

            default: {
                auto p = new (buf) fixed_packer<std::intmax_t>{};
                p->value = value;
                break;
            }
        }
    }
}

inline std::pair<bool,std::intmax_t> get_integer (blob_t const & blob)
{
    if (blob.size() == sizeof(std::intmax_t)) {
        fixed_packer<std::intmax_t> p;
        std::memcpy(p.bytes, blob.data(), blob.size());
        return std::make_pair(true, static_cast<std::intmax_t>(p.value));
    } else {
        switch (blob.size()) {
            case sizeof(std::int8_t): {
                fixed_packer<std::int8_t> p;
                std::memcpy(p.bytes, blob.data(), blob.size());
                return std::make_pair(true, static_cast<std::intmax_t>(p.value));
            }

            case sizeof(std::int16_t): {
                fixed_packer<std::int16_t> p;
                std::memcpy(p.bytes, blob.data(), blob.size());
                return std::make_pair(true, static_cast<std::intmax_t>(p.value));
            }

            case sizeof(std::int32_t): {
                fixed_packer<std::int32_t> p;
                std::memcpy(p.bytes, blob.data(), blob.size());
                return std::make_pair(true, static_cast<std::intmax_t>(p.value));
            }

            case sizeof(std::int64_t): {
                fixed_packer<std::int64_t> p;
                std::memcpy(p.bytes, blob.data(), blob.size());
                return std::make_pair(true, static_cast<std::intmax_t>(p.value));
            }

            default:
                // Error: unsuitable or corrupted data
                break;
        }
    }

    return std::make_pair(false, 0);
}

inline std::pair<bool,float> get_float (blob_t const & blob)
{
    if (blob.size() == sizeof(float)) {
        fixed_packer<float> p;
        std::memcpy(p.bytes, blob.data(), blob.size());

        if (!std::isnan(p.value))
            return std::make_pair(true, p.value);
    }

    return std::make_pair(false, float{0});
}

inline std::pair<bool,double> get_double (blob_t const & blob)
{
    if (blob.size() == sizeof(double)) {
        fixed_packer<double> p;
        std::memcpy(p.bytes, blob.data(), blob.size());

        if (!std::isnan(p.value))
            return std::make_pair(true, p.value);
    }

    return std::make_pair(false, double{0});
}

}} // namespace debby::backend


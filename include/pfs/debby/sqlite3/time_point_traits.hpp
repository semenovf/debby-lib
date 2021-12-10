////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/time_point.hpp"
#include "affinity_traits.hpp"
#include "cast_traits.hpp"

namespace pfs {
namespace debby {
namespace sqlite3 {

template <> struct affinity_traits<time_point> : integral64_affinity_traits {};
template <> struct affinity_traits<time_point &> : integral64_affinity_traits {};
template <> struct affinity_traits<time_point const> : integral64_affinity_traits {};
template <> struct affinity_traits<time_point const &> : integral64_affinity_traits {};

template <> struct affinity_traits<utc_time_point> : integral64_affinity_traits {};
template <> struct affinity_traits<utc_time_point &> : integral64_affinity_traits {};
template <> struct affinity_traits<utc_time_point const> : integral64_affinity_traits {};
template <> struct affinity_traits<utc_time_point const &> : integral64_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, time_point>::value, void>::type>
{
    using storage_type = typename affinity_traits<time_point>::storage_type;

    static storage_type to_storage (time_point const & value)
    {
        return to_millis(value).count();
    }

    static optional<time_point> to_native (storage_type const & value)
    {
        return from_millis(std::chrono::milliseconds{value});
    }
};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, utc_time_point>::value, void>::type>
{
    using storage_type = typename affinity_traits<utc_time_point>::storage_type;

    static storage_type to_storage (utc_time_point const & value)
    {
        return to_millis(value.value).count();
    }

    static optional<utc_time_point> to_native (storage_type const & value)
    {
        return utc_time_point{from_millis(std::chrono::milliseconds{value})};
    }
};

}}} // namespace pfs::debby::sqlite3

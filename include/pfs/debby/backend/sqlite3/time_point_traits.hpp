////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/time_point.hpp"
#include "affinity_traits.hpp"
#include "cast_traits.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

template <> struct affinity_traits<pfs::local_time_point> : integral64_affinity_traits {};
template <> struct affinity_traits<pfs::local_time_point &> : integral64_affinity_traits {};
template <> struct affinity_traits<pfs::local_time_point const> : integral64_affinity_traits {};
template <> struct affinity_traits<pfs::local_time_point const &> : integral64_affinity_traits {};

template <> struct affinity_traits<pfs::utc_time_point> : integral64_affinity_traits {};
template <> struct affinity_traits<pfs::utc_time_point &> : integral64_affinity_traits {};
template <> struct affinity_traits<pfs::utc_time_point const> : integral64_affinity_traits {};
template <> struct affinity_traits<pfs::utc_time_point const &> : integral64_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<typename std::decay<NativeType>::type, pfs::local_time_point>::value, void>::type>
{
    using storage_type = typename affinity_traits<pfs::local_time_point>::storage_type;

    static storage_type to_storage (pfs::local_time_point const & value)
    {
        return value.to_millis().count();
    }

    static pfs::local_time_point to_native (storage_type const & value)
    {
        return pfs::local_time_point{std::chrono::milliseconds{value}};
    }
};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<typename std::decay<NativeType>::type, pfs::utc_time_point>::value, void>::type>
{
    using storage_type = typename affinity_traits<pfs::utc_time_point>::storage_type;

    static storage_type to_storage (pfs::utc_time_point const & value)
    {
        return value.to_millis().count();
    }

    static pfs::utc_time_point to_native (storage_type const & value)
    {
        return pfs::utc_time_point{std::chrono::milliseconds{value}};
    }
};

}}} // namespace debby::backend::sqlite3

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "cast_traits.hpp"
#include "pfs/universal_id.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

template <> struct affinity_traits<pfs::universal_id> : text_affinity_traits {};
template <> struct affinity_traits<pfs::universal_id &> : text_affinity_traits {};
template <> struct affinity_traits<pfs::universal_id const> : text_affinity_traits {};
template <> struct affinity_traits<pfs::universal_id const &> : text_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, pfs::universal_id>::value, void>::type>
{
    using storage_type = typename affinity_traits<pfs::universal_id>::storage_type;

    static storage_type to_storage (pfs::universal_id const & value)
    {
        return std::to_string(value);
    }

    static pfs::universal_id to_native (storage_type const & value)
    {
        return pfs::from_string<pfs::universal_id>(value);
    }
};

}}} // namespace debby::backend::sqlite3

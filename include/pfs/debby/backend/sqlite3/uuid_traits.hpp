////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/uuid.hpp"
#include "affinity_traits.hpp"
#include "cast_traits.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

template <> struct affinity_traits<pfs::uuid_t> : text_affinity_traits {};
template <> struct affinity_traits<pfs::uuid_t &> : text_affinity_traits {};
template <> struct affinity_traits<pfs::uuid_t const> : text_affinity_traits {};
template <> struct affinity_traits<pfs::uuid_t const &> : text_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, pfs::uuid_t>::value, void>::type>
{
    using storage_type = typename affinity_traits<pfs::uuid_t>::storage_type;

    static storage_type to_storage (pfs::uuid_t const & value)
    {
        return std::to_string(value);
    }

    static pfs::optional<pfs::uuid_t> to_native (storage_type const & value)
    {
        return pfs::from_string<pfs::uuid_t>(value);
    }
};

}}} // namespace debby::backend::sqlite3
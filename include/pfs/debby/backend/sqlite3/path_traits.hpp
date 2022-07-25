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
#include "pfs/filesystem.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

template <> struct affinity_traits<pfs::filesystem::path> : text_affinity_traits {};
template <> struct affinity_traits<pfs::filesystem::path &> : text_affinity_traits {};
template <> struct affinity_traits<pfs::filesystem::path const> : text_affinity_traits {};
template <> struct affinity_traits<pfs::filesystem::path const &> : text_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, pfs::filesystem::path>::value, void>::type>
{
    using storage_type = typename affinity_traits<pfs::filesystem::path>::storage_type;

    static storage_type to_storage (pfs::filesystem::path const & value)
    {
        return pfs::filesystem::utf8_encode(value);
    }

    static pfs::filesystem::path to_native (storage_type const & value)
    {
        return pfs::filesystem::utf8_decode(value);
    }
};

}}} // namespace debby::backend::sqlite3


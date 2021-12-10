////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/type_traits.hpp"
#include <cstdint>
#include <string>

namespace pfs {
namespace debby {
namespace sqlite3 {

struct integral_affinity
{
    using storage_type = int;
    static std::string name () { return "INTEGER"; }
};

struct integral64_affinity
{
    using storage_type = std::int64_t;
    static std::string name () { return "INTEGER"; }
};

struct floating_affinity
{
    using storage_type = double;
    static std::string name () { return "REAL"; }
};

struct text_affinity
{
    using storage_type = std::string;
    static std::string name () { return "TEXT"; }
};

template <typename NativeType, typename Constraints = void>
struct affinity;

template <typename NativeType>
struct affinity<NativeType, typename std::enable_if<
       std::is_integral<pfs::remove_cvref_t<NativeType>>::value
    && !std::is_same<pfs::remove_cvref_t<NativeType>, std::int64_t>::value
    && !std::is_same<pfs::remove_cvref_t<NativeType>, std::uint64_t>::value>::type> : integral_affinity
{};

template <typename NativeType>
struct affinity<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, std::int64_t>::value
    || std::is_same<pfs::remove_cvref_t<NativeType>, std::uint64_t>::value>::type> : integral64_affinity
{};

template <typename NativeType>
struct affinity<NativeType, typename std::enable_if<
    std::is_floating_point<pfs::remove_cvref_t<NativeType>>::value>::type> : floating_affinity
{};

template <typename NativeType>
struct affinity<NativeType, typename std::enable_if<
    std::is_same<pfs::remove_cvref_t<NativeType>, std::string>::value>::type> : text_affinity
{};

}}} // namespace pfs::debby::sqlite3


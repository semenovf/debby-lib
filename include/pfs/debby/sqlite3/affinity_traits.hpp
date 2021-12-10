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
#include "pfs/string_view.hpp"
#include "pfs/optional.hpp"
#include <cstdint>
#include <string>

namespace pfs {
namespace debby {
namespace sqlite3 {

// The preferred storage class for a column is called its "affinity".

struct integral_affinity_traits
{
    using storage_type = std::int32_t;
    static std::string name () { return "INTEGER"; }
};

struct integral64_affinity_traits
{
    using storage_type = std::int64_t;
    static std::string name () { return "INTEGER"; }
};

struct floating_affinity_traits
{
    using storage_type = double;
    static std::string name () { return "REAL"; }
};

struct text_affinity_traits
{
    using storage_type = std::string;
    static std::string name () { return "TEXT"; }
};

template <typename NativeType, typename Constraints = void>
struct affinity_traits;

template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
       std::is_integral<pfs::remove_cvref_t<NativeType>>::value
       && sizeof(NativeType) <= 4, void>::type>
    : integral_affinity_traits
{};

template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
        std::is_integral<pfs::remove_cvref_t<NativeType>>::value
        && (sizeof(NativeType) > 4 && sizeof(NativeType) <= 8), void>::type>
    : integral64_affinity_traits
{};

template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_floating_point<pfs::remove_cvref_t<NativeType>>::value, void>::type>
    : floating_affinity_traits
{};

template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<pfs::remove_cvref_t<NativeType>, std::string>::value, void>::type>
    : text_affinity_traits
{};

template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<pfs::remove_cvref_t<NativeType>, pfs::string_view>::value, void>::type>
    : text_affinity_traits
{};

template <typename NativeType>
struct affinity_traits<optional<NativeType>, void>: affinity_traits<NativeType> {};

}}} // namespace pfs::debby::sqlite3


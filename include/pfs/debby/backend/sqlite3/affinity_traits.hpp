////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
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
 #include <vector>

namespace debby {
namespace backend {
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

struct blob_affinity_traits
{
    using storage_type = std::vector<std::uint8_t>;
    static std::string name () { return "BLOB"; }
};

template <typename NativeType, typename Constraints = void>
struct affinity_traits;

// Affinity traits for integers
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
       std::is_integral<pfs::remove_cvref_t<NativeType>>::value
       && sizeof(NativeType) <= 4, void>::type>
    : integral_affinity_traits
{};

// Affinity traits for 64-bit integers
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
        std::is_integral<pfs::remove_cvref_t<NativeType>>::value
        && (sizeof(NativeType) > 4 && sizeof(NativeType) <= 8), void>::type>
    : integral64_affinity_traits
{};

// Affinity traits for floating point values
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_floating_point<pfs::remove_cvref_t<NativeType>>::value, void>::type>
    : floating_affinity_traits
{};

// Affinity traits for std::string
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<pfs::remove_cvref_t<NativeType>, std::string>::value, void>::type>
    : text_affinity_traits
{};

// Affinity traits for string_view
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<pfs::remove_cvref_t<NativeType>, pfs::string_view>::value, void>::type>
    : text_affinity_traits
{};

// Affinity traits for C-style strings
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<NativeType, char const *>::value, void>::type>
    : text_affinity_traits
{};

// Affinity traits for blobs
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<pfs::remove_cvref_t<NativeType>, std::vector<std::uint8_t>>::value, void>::type>
    : blob_affinity_traits
{};

// Affinity traits for enumerations
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_enum<pfs::remove_cvref_t<NativeType>>::value, void>::type>
    : affinity_traits<typename std::underlying_type<pfs::remove_cvref_t<NativeType>>::type>
{};

template <typename NativeType>
struct affinity_traits<pfs::optional<NativeType>, void>: affinity_traits<NativeType> {};

}}} // namespace debby::backend::sqlite3

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
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
namespace psql {

// The preferred storage class for a column is called its "affinity".

struct boolean_affinity_traits
{
    using storage_type = bool;
    static std::string name () { return "BOOLEAN"; }
};

struct integral16_affinity_traits
{
    using storage_type = std::int16_t;
    static std::string name () { return "SMALLINT"; }
};

struct integral32_affinity_traits
{
    using storage_type = std::int32_t;
    static std::string name () { return "INTEGER"; }
};

struct integral64_affinity_traits
{
    using storage_type = std::int64_t;
    static std::string name () { return "BIGINT"; }
};

struct float_affinity_traits
{
    using storage_type = float;
    static std::string name () { return "REAL"; }
};

struct double_affinity_traits
{
    using storage_type = double;
    static std::string name () { return "DOUBLE PRECISION"; }
};

struct text_affinity_traits
{
    using storage_type = std::string;
    static std::string name () { return "TEXT"; }
};

struct blob_affinity_traits
{
    using storage_type = std::vector<std::uint8_t>;
    static std::string name () { return "BYTEA"; }
};

template <typename NativeType, typename Constraints = void>
struct affinity_traits;

template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<typename std::decay<NativeType>::type, bool>::value, void>::type>
    : boolean_affinity_traits
{};

// Affinity traits for integers
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
       std::is_integral<typename std::decay<NativeType>::type>::value
       && !std::is_same<typename std::decay<NativeType>::type, bool>::value
       && sizeof(NativeType) <= 2, void>::type>
    : integral16_affinity_traits
{};

// Affinity traits for integers
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
       std::is_integral<typename std::decay<NativeType>::type>::value
       && (sizeof(NativeType) > 2 && sizeof(NativeType) <= 4), void>::type>
    : integral32_affinity_traits
{};

// Affinity traits for 64-bit integers
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
        std::is_integral<typename std::decay<NativeType>::type>::value
        && (sizeof(NativeType) > 4 && sizeof(NativeType) <= 8), void>::type>
    : integral64_affinity_traits
{};

// Affinity traits for floating point values
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<typename std::decay<NativeType>::type, float>::value, void>::type>
    : float_affinity_traits
{};

template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<typename std::decay<NativeType>::type, double>::value, void>::type>
    : double_affinity_traits
{};

// Affinity traits for std::string
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<typename std::decay<NativeType>::type, std::string>::value, void>::type>
    : text_affinity_traits
{};

// Affinity traits for string_view
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_same<typename std::decay<NativeType>::type, pfs::string_view>::value, void>::type>
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
    std::is_same<typename std::decay<NativeType>::type, std::vector<std::uint8_t>>::value, void>::type>
    : blob_affinity_traits
{};

// Affinity traits for enumerations
template <typename NativeType>
struct affinity_traits<NativeType, typename std::enable_if<
    std::is_enum<typename std::decay<NativeType>::type>::value, void>::type>
    : affinity_traits<typename std::underlying_type<typename std::decay<NativeType>::type>::type>
{};

template <typename NativeType>
struct affinity_traits<pfs::optional<NativeType>, void>: affinity_traits<NativeType> {};

}}} // namespace debby::backend::psql

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
#include "pfs/string_view.hpp"
#include "pfs/utility.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

template <typename NativeType, typename Constraints = void>
struct cast_traits;

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
    std::is_arithmetic<typename std::decay<NativeType>::type>::value, void>::type>
{
    using storage_type = typename affinity_traits<NativeType>::storage_type;

    static storage_type to_storage (NativeType const & value)
    {
        return static_cast<storage_type>(value);
    }

    static NativeType to_native (storage_type const & value)
    {
        return static_cast<NativeType>(value);
    }
};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<typename std::decay<NativeType>::type, std::string>::value, void>::type>
{
    using storage_type = typename affinity_traits<NativeType>::storage_type;

    static storage_type to_storage (NativeType const & value)
    {
        return value;
    }

    static NativeType to_native (storage_type const & value)
    {
        return value;
    }
};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
    std::is_same<typename std::decay<NativeType>::type, pfs::string_view>::value, void>::type>
{
    using storage_type = typename affinity_traits<NativeType>::storage_type;

    static storage_type to_storage (NativeType const & value)
    {
        return value.to_string();
    }

    // Avoid unsafe conversion from std::string to string_view
    // static NativeType to_native (storage_type const & value)
};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<typename std::decay<NativeType>::type, std::vector<std::uint8_t>>::value, void>::type>
{
    using storage_type = typename affinity_traits<NativeType>::storage_type;

    static storage_type to_storage (NativeType const & value)
    {
        return value;
    }

    static NativeType to_native (storage_type const & value)
    {
        return value;
    }
};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<typename std::decay<NativeType>::type, std::vector<char>>::value, void>::type>
{
    using storage_type = typename affinity_traits<NativeType>::storage_type;

    static storage_type to_storage (NativeType const & value)
    {
        return value;
    }

    static NativeType to_native (storage_type const & value)
    {
        return value;
    }
};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_enum<typename std::decay<NativeType>::type>::value, void>::type>
{
    using storage_type = typename affinity_traits<NativeType>::storage_type;

    static storage_type to_storage (NativeType const & value)
    {
        return pfs::to_underlying(value);
    }

    static NativeType to_native (storage_type const & value)
    {
        return static_cast<NativeType>(value);
    }
};

template <typename NativeType>
struct cast_traits<pfs::optional<NativeType>, void>: cast_traits<NativeType> {};

template <typename NativeType>
inline typename affinity_traits<NativeType>::storage_type to_storage (NativeType const & value)
{
    return cast_traits<NativeType>::to_storage(value);
}

template <typename NativeType>
inline NativeType to_native (typename affinity_traits<NativeType>::storage_type const & value)
{
    return cast_traits<NativeType>::to_native(value);
}

inline std::nullptr_t to_storage (std::nullptr_t)
{
    return nullptr;
}

inline char const * to_storage (char const * value)
{
    return value;
}

}}} // namespace debby::backend::sqlite3

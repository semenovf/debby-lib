////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.04 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "backend_enum.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "keyvalue_database.hpp"

#if DEBBY__MAP_ENABLED
#   include <map>
#endif

#if DEBBY__UNORDERED_MAP_ENABLED
#   include <unordered_map>
#endif

DEBBY__NAMESPACE_BEGIN

namespace in_memory {

template <backend_enum Backend>
DEBBY__EXPORT keyvalue_database<Backend> make_kv (error * perr = nullptr);

template <backend_enum Backend>
DEBBY__EXPORT bool wipe (error * perr = nullptr);

} // namespace in_memory

#if DEBBY__MAP_ENABLED
template <>
template <typename ...Args>
keyvalue_database<backend_enum::map_st>
keyvalue_database<backend_enum::map_st>::make (Args &&... args)
{
    return keyvalue_database<backend_enum::map_st> {
        in_memory::make_kv<backend_enum::map_st>(std::forward<Args>(args)...)
    };
}

template <>
template <typename ...Args>
keyvalue_database<backend_enum::map_mt>
keyvalue_database<backend_enum::map_mt>::make (Args &&... args)
{
    return keyvalue_database<backend_enum::map_mt> {
        in_memory::make_kv<backend_enum::map_mt>(std::forward<Args>(args)...)
    };
}

template <>
template <typename ...Args>
bool
keyvalue_database<backend_enum::map_st>::wipe (Args &&... args)
{
    return in_memory::wipe<backend_enum::map_st>(std::forward<Args>(args)...);
}

template <>
template <typename ...Args>
bool
keyvalue_database<backend_enum::map_mt>::wipe (Args &&... args)
{
    return in_memory::wipe<backend_enum::map_mt>(std::forward<Args>(args)...);
}
#endif

#if DEBBY__UNORDERED_MAP_ENABLED
template <>
template <typename ...Args>
keyvalue_database<backend_enum::unordered_map_st>
keyvalue_database<backend_enum::unordered_map_st>::make (Args &&... args)
{
    return keyvalue_database<backend_enum::unordered_map_st> {
        in_memory::make_kv<backend_enum::unordered_map_st>(std::forward<Args>(args)...)
    };
}

template <>
template <typename ...Args>
keyvalue_database<backend_enum::unordered_map_mt>
keyvalue_database<backend_enum::unordered_map_mt>::make (Args &&... args)
{
    return keyvalue_database<backend_enum::unordered_map_mt> {
        in_memory::make_kv<backend_enum::unordered_map_mt>(std::forward<Args>(args)...)
    };
}

template <>
template <typename ...Args>
bool
keyvalue_database<backend_enum::unordered_map_st>::wipe (Args &&... args)
{
    return in_memory::wipe<backend_enum::unordered_map_st>(std::forward<Args>(args)...);
}

template <>
template <typename ...Args>
bool
keyvalue_database<backend_enum::unordered_map_mt>::wipe (Args &&... args)
{
    return in_memory::wipe<backend_enum::unordered_map_mt>(std::forward<Args>(args)...);
}
#endif

DEBBY__NAMESPACE_END

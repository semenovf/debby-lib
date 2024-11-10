////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.05 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "keyvalue_database.hpp"
#include <pfs/filesystem.hpp>
#include <cstdint>

DEBBY__NAMESPACE_BEGIN

namespace lmdb {

struct options_type
{
    std::uint32_t env;
    std::uint32_t db;
};

/**
 * Open database specified by @a path and create it if
 * missing when @a create_if_missing set to @c true.
 *
 * @param path Path to the database.
 *
 * @throw debby::error().
 */
DEBBY__EXPORT
keyvalue_database<backend_enum::lmdb>
make_kv (pfs::filesystem::path const & path, options_type opts, bool create_if_missing, error * perr = nullptr);

DEBBY__EXPORT
keyvalue_database<backend_enum::lmdb>
make_kv (pfs::filesystem::path const & path, bool create_if_missing, error * perr = nullptr);

DEBBY__EXPORT bool wipe (pfs::filesystem::path const & path, error * perr = nullptr);

} // namespace lmdb

template<>
template <typename ...Args>
keyvalue_database<backend_enum::lmdb>
keyvalue_database<backend_enum::lmdb>::make (Args &&... args)
{
    return keyvalue_database<backend_enum::lmdb>{lmdb::make_kv(std::forward<Args>(args)...)};
}

/**
 * Deletes files associated with database
 */
template<>
template <typename ...Args>
bool keyvalue_database<backend_enum::lmdb>::wipe (Args &&... args)
{
    return lmdb::wipe(std::forward<Args>(args)...);
}

DEBBY__NAMESPACE_END

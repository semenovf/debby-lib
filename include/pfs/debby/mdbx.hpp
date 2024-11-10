////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "error.hpp"
#include "exports.hpp"
#include "keyvalue_database.hpp"
#include <pfs/filesystem.hpp>
#include <cstdint>

DEBBY__NAMESPACE_BEGIN

namespace mdbx {

struct options_type 
{
    std::uint32_t env; // See MDBX_env_flags_t
    std::uint32_t db;  // See MDBX_db_flags_t
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
keyvalue_database<backend_enum::mdbx>
make_kv (pfs::filesystem::path const & path, options_type opts, bool create_if_missing, error * perr = nullptr);

DEBBY__EXPORT
keyvalue_database<backend_enum::mdbx>
make_kv (pfs::filesystem::path const & path, bool create_if_missing, error * perr = nullptr);

DEBBY__EXPORT bool wipe (pfs::filesystem::path const & path, error * perr = nullptr);

} // namespace mdbx

template<>
template <typename ...Args>
keyvalue_database<backend_enum::mdbx>
keyvalue_database<backend_enum::mdbx>::make (Args &&... args)
{
    return keyvalue_database<backend_enum::mdbx>{mdbx::make_kv(std::forward<Args>(args)...)};
}

/**
 * Deletes files associated with database
 */
template<>
template <typename ...Args>
bool keyvalue_database<backend_enum::mdbx>::wipe (Args &&... args)
{
    return mdbx::wipe(std::forward<Args>(args)...);
}

DEBBY__NAMESPACE_END

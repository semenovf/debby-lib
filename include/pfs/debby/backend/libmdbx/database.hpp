////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.06 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/filesystem.hpp"

struct MDBX_env;

namespace debby {
namespace backend {
namespace libmdbx {

struct database
{
    using key_type     = std::string;
    using native_type  = std::uint32_t; // Must be same type as MDBX_dbi

    struct options_type {
        std::uint32_t env; // See MDBX_env_flags_t
        std::uint32_t db;  // See MDBX_db_flags_t
    };

    struct rep_type
    {
        MDBX_env * env {nullptr};
        native_type dbh;
        pfs::filesystem::path path;
    };

    /**
     * Open database specified by @a path and create it if
     * missing when @a create_if_missing set to @c true.
     *
     * @param path Path to the database.
     *
     * @throw debby::error().
     */
    static DEBBY__EXPORT rep_type make_kv (pfs::filesystem::path const & path
        , options_type opts, error * perr = nullptr);

    static DEBBY__EXPORT rep_type make_kv (pfs::filesystem::path const & path
        , bool create_if_missing, error * perr = nullptr);
};

}}} // namespace debby::backend::libmdbx

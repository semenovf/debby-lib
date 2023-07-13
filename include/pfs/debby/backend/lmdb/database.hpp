////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.07.13 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/filesystem.hpp"
#include <string>

struct MDB_env;

namespace debby {
namespace backend {
namespace lmdb {

struct database
{
    using key_type     = std::string;
    using native_type  = unsigned int; // Must be same type as MDB_dbi

    struct options_type {
        std::uint32_t env;
        std::uint32_t db;
    };

    struct rep_type
    {
        MDB_env * env {nullptr};
        native_type dbh {0};
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

}}} // namespace debby::backend::lmdb

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2025.10.12 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "backend_enum.hpp"
#include "error.hpp"
#include <pfs/filesystem.hpp>

DEBBY__NAMESPACE_BEGIN

template <>
inline char const * column_type_affinity<backend_enum::sqlite3, pfs::filesystem::path>::type ()
{
    return "TEXT";
}

template <>
inline char const * column_type_affinity<backend_enum::psql, pfs::filesystem::path>::type ()
{
    return "TEXT";
}

template <>
struct value_type_affinity<pfs::filesystem::path>
{
    using affinity_type = std::string;

    static affinity_type cast (pfs::filesystem::path const & value)
    {
        return pfs::utf8_encode_path(value);
    }

    static pfs::filesystem::path cast (affinity_type const & str, error *)
    {
        return pfs::utf8_decode_path(str);
    }
};

DEBBY__NAMESPACE_END


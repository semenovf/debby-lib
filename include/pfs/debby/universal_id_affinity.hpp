////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2025.09.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "backend_enum.hpp"
#include "error.hpp"
#include <pfs/universal_id.hpp>

DEBBY__NAMESPACE_BEGIN

template <>
inline char const * column_type_affinity<backend_enum::sqlite3, pfs::universal_id>::type ()
{
    return "TEXT";
}

template <>
inline char const * column_type_affinity<backend_enum::psql, pfs::universal_id>::type ()
{
    return "CHARACTER(26)";
}

template <>
struct keyvalue_affinity<pfs::universal_id>
{
    using affinity_type = std::string;

    static affinity_type cast (pfs::universal_id const & value)
    {
        return to_string(value);
    }

    static pfs::universal_id cast (affinity_type const & str, error * perr)
    {
        auto uid = pfs::parse_universal_id(str);

        if (!uid) {
            pfs::throw_or(perr, make_error_code(errc::bad_value));
            return pfs::universal_id{};
        }

        return *uid;
    }
};

DEBBY__NAMESPACE_END

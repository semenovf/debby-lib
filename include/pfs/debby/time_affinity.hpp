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
#include <pfs/time_point.hpp>
#include <cstdint>

DEBBY__NAMESPACE_BEGIN

template <>
inline char const * column_type_affinity<backend_enum::sqlite3, pfs::utc_time>::type ()
{
    return "INTEGER";
}

template <>
inline char const * column_type_affinity<backend_enum::sqlite3, pfs::local_time>::type ()
{
    return "INTEGER";
}

template <>
inline char const * column_type_affinity<backend_enum::psql, pfs::utc_time>::type ()
{
    return "INTEGER";
}

template <>
inline char const * column_type_affinity<backend_enum::psql, pfs::local_time>::type ()
{
    return "INTEGER";
}

template <>
struct value_type_affinity<pfs::utc_time>
{
    using affinity_type = std::int64_t;

    static affinity_type cast (pfs::utc_time const & value)
    {
        return value.to_millis().count();
    }

    static pfs::utc_time cast (affinity_type millis, error * perr)
    {
        return pfs::utc_time {std::chrono::milliseconds{millis}};
    }
};

template <>
struct value_type_affinity<pfs::local_time>
{
    using affinity_type = std::int64_t;

    static affinity_type cast (pfs::local_time const & value)
    {
        return value.to_millis().count();
    }

    static pfs::local_time cast (affinity_type millis, error * perr)
    {
        return pfs::local_time {std::chrono::milliseconds{millis}};
    }
};

DEBBY__NAMESPACE_END


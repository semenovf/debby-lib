////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.02 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "debby/namespace.hpp"
#include "debby/result.hpp"

extern "C" {
#include <libpq-fe.h>
}

DEBBY__NAMESPACE_BEGIN

using result_t = result<backend_enum::psql>;

template <>
class result_t::impl
{
public:
    using handle_type = struct pg_result *;

public:
    mutable handle_type sth {nullptr};
    int row_count {0}; // Total number of tuples
    int row_index {0};

public:
    impl (handle_type h)
        : sth(h)
    {
        row_count = PQntuples(sth);
    }

    impl (impl && other)
    {
        sth = other.sth;
        row_count = other.row_count;
        row_index = other.row_index;

        other.sth = nullptr;
    }

    ~impl ()
    {
        if (sth != nullptr)
            PQclear(sth);

        sth = nullptr;
    }
};

DEBBY__NAMESPACE_END

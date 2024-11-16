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
    int field_count {0}; // Number of fields
    int row_count {0};  // Total number of tuples
    int row_index {0};

public:
    impl (handle_type h)
        : sth(h)
    {
        field_count = PQnfields(sth);
        row_count = PQntuples(sth);
    }

    impl (impl && other)
    {
        sth = other.sth;
        field_count = other.field_count;
        row_count  = other.row_count;
        row_index  = other.row_index;

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

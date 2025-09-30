////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.02 Initial version.
//      2025.09.30 Changed get implementation.
////////////////////////////////////////////////////////////////////////////////
#include "debby/namespace.hpp"
#include "debby/result.hpp"
#include <pfs/optional.hpp>

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
    int column_count {0}; // Number of fields
    int row_count {0};    // Total number of tuples
    int row_index {0};

public:
    impl (handle_type h)
        : sth(h)
    {
        column_count = PQnfields(sth);
        row_count = PQntuples(sth);
    }

    impl (impl && other)
    {
        sth = other.sth;
        column_count = other.column_count;
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

public:
    // NOTE result's API expects that column index starts from 1, but internally it starts from 0.
    //
    pfs::optional<std::int64_t> get_int64 (int column, error * perr);
    pfs::optional<double> get_double (int column, error * perr);
    pfs::optional<std::string> get_string (int column, error * perr);
    pfs::optional<std::int64_t> get_int64 (std::string const & column_name, error * perr);
    pfs::optional<double> get_double (std::string const & column_name, error * perr);
    pfs::optional<std::string> get_string (std::string const & column_name, error * perr);

private:
    int column_index (std::string const & column_name)
    {
        for (int i = 0; i < column_count; i++) {
            auto name = PQfname(sth, i);

            if (column_name == name)
                return i;
        }

        return -1;
    }
};

DEBBY__NAMESPACE_END

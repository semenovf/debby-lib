////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.10.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "debby/relational_database.hpp"

DEBBY__NAMESPACE_BEGIN

template <backend_enum Backend>
relational_database<Backend>::relational_database ()
{}

template <backend_enum Backend>
relational_database<Backend>::relational_database (impl && d) noexcept
    : _d(std::make_unique<impl>(std::move(d)))
{}

template <backend_enum Backend>
relational_database<Backend>::relational_database (relational_database && other) noexcept
    : _d(std::move(other._d))
{}

template <backend_enum Backend>
relational_database<Backend>::~relational_database ()
{}

template <backend_enum Backend>
relational_database<Backend> & relational_database<Backend>::operator = (relational_database && other) noexcept
{
    _d = std::move(other._d);
    return *this;
}

template <backend_enum Backend>
std::size_t
relational_database<Backend>::rows_count (std::string const & table_name, error * perr)
{
    std::size_t count = 0;
    std::string sql = fmt::format("SELECT COUNT(1) as count FROM \"{}\"", table_name);

    auto res = exec(sql, perr);

    if (res) {
        if (res.has_more()) {
            count = *res.template get<std::size_t>(1);

            // May be `sql_error` exception
            res.next();
        }
    }

    // Expecting done
    PFS__ASSERT(res.is_done(), "expecting done");

    return count;
}

DEBBY__NAMESPACE_END

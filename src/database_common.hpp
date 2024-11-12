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
relational_database<Backend>::relational_database () = default;

template <backend_enum Backend>
relational_database<Backend>::relational_database (impl && d)
{
    _d = new impl(std::move(d));
}

template <backend_enum Backend>
relational_database<Backend>::relational_database (relational_database && other) noexcept
{
    _d = other._d;
    other._d = nullptr;
}

template <backend_enum Backend>
relational_database<Backend>::~relational_database ()
{
    if (_d != nullptr)
        delete _d;

    _d = nullptr;
}

template <backend_enum Backend>
relational_database<Backend> & relational_database<Backend>::operator = (relational_database && other) noexcept
{
    if (_d != nullptr)
        delete _d;

    _d = other._d;
    other._d = nullptr;

    return *this;
}

template <backend_enum Backend>
std::size_t
relational_database<Backend>::rows_count (std::string const & table_name, error * perr)
{
    std::size_t count = 0;
    std::string sql = fmt::format("SELECT COUNT(1) as count FROM \"{}\"", table_name);

    statement_type stmt = prepare(sql, false, perr);

    if (stmt) {
        auto res = stmt.exec();

        if (res) {
            if (res.has_more()) {
                count = res.template get<std::size_t>(0);

                // May be `sql_error` exception
                res.next();
            }
        }

        // Expecting done
        PFS__ASSERT(res.is_done(), "expecting done");
    }

    return count;
}

DEBBY__NAMESPACE_END

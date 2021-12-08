////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_database.hpp"
#include <string>
#include <vector>

namespace pfs {
namespace debby {

template <typename Impl, typename Traits>
class relational_database : public basic_database<Impl>
{
    using statement_type = typename Traits::statement_type;

public:
    bool query (std::string const & sql)
    {
        return static_cast<Impl *>(this)->query_impl(sql);
    }

    bool begin ()
    {
        return static_cast<Impl *>(this)->begin_impl();
    }

    bool commit ()
    {
        return static_cast<Impl *>(this)->commit_impl();
    }

    bool rollback ()
    {
        return static_cast<Impl *>(this)->rollback_impl();
    }

    /**
     * Lists available tables at database by pattern.
     */
    std::vector<std::string> tables (std::string const & pattern = std::string{})
    {
        return static_cast<Impl *>(this)->tables_impl(pattern);
    }

    /**
     * Checks if named table exists at database.
     */
    bool exists (std::string const & name)
    {
        return static_cast<Impl *>(this)->exists_impl(name);
    }

    statement_type prepare (std::string const & sql, bool cache = true)
    {
        return static_cast<Impl*>(this)->prepare_impl(sql, cache);
    }
};

}} // namespace pfs::debby


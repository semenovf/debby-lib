////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/filesystem.hpp"
#include <string>
#include <vector>

namespace pfs {
namespace debby {

template <typename Impl>
class basic_database
{
public:
    std::string last_error () const noexcept
    {
        return static_cast<Impl const *>(this)->last_error_impl();
    }

    bool open (filesystem::path const & path)
    {
        return static_cast<Impl*>(this)->open_impl(path);
    }

    void close ()
    {
        static_cast<Impl*>(this)->close_impl();
    }

    bool is_opened () const noexcept
    {
        return static_cast<Impl const *>(this)->is_opened_impl();
    }

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
     * Drop database (delete all tables)
     */
    bool clear ()
    {
        return static_cast<Impl *>(this)->clear_impl();
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
};

}} // namespace pfs::debby

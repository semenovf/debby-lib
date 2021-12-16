////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.16 Changed methods signatures.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/filesystem.hpp"
#include "pfs/debby/error.hpp"
#include <functional>
#include <string>
#include <vector>

namespace pfs {
namespace debby {

template <typename Impl>
class basic_database
{
protected:
    basic_database () = default;
    basic_database (basic_database const &) = delete;
    basic_database & operator = (basic_database const &) = delete;
    basic_database (basic_database && other) = default;
    basic_database & operator = (basic_database && other) = default;

    ~basic_database ()
    {
        close();
    }

public:
    /**
     * Open database specified by @a path and create it if
     * missing when @a create_if_missing set to @c true.
     */
    std::error_code open (filesystem::path const & path, bool create_if_missing = true)
    {
        return static_cast<Impl*>(this)->open_impl(path, create_if_missing);
    }

    /**
     * Close database.
     */
    std::error_code close ()
    {
        return static_cast<Impl*>(this)->close_impl();
    }

    /**
     * Checks if database is open.
     */
    bool is_opened () const noexcept
    {
        return static_cast<Impl const *>(this)->is_opened_impl();
    }
};

}} // namespace pfs::debby

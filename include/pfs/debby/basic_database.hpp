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
    /**
     * Open database specified by @a path.
     *
     * @return @c true on success, @c false on failure
     *
     * @throw pfs::bad_alloc
     * @throw pfs::runtime_error
     */
    bool open (filesystem::path const & path, bool create_if_missing = false)
    {
        return static_cast<Impl*>(this)->open_impl(path, create_if_missing);
    }

    void close ()
    {
        static_cast<Impl*>(this)->close_impl();
    }

    bool is_opened () const noexcept
    {
        return static_cast<Impl const *>(this)->is_opened_impl();
    }
};

}} // namespace pfs::debby

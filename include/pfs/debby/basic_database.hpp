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
};

}} // namespace pfs::debby

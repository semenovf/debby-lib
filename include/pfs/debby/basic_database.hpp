////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.16 Changed methods signatures.
//      2021.12.17 Removed deprecated methods.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/filesystem.hpp"
#include <functional>
#include <string>
#include <vector>

namespace debby {

template <typename Impl>
class basic_database
{
protected:
    basic_database () = default;
    ~basic_database () = default;
    basic_database (basic_database const &) = delete;
    basic_database & operator = (basic_database const &) = delete;
    basic_database (basic_database && other) = default;
    basic_database & operator = (basic_database && other) = default;

public:
    /**
     * Checks if database is open.
     */
    operator bool () const noexcept
    {
        return static_cast<Impl const *>(this)->is_opened();
    }
};

} // namespace debby

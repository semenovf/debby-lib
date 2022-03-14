////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "result.hpp"
#include "cast_traits.hpp"
#include "time_point_traits.hpp"
#include "uuid_traits.hpp"
#include "pfs/debby/result.hpp"
#include "pfs/type_traits.hpp"

struct sqlite3_stmt;

namespace debby {
namespace backend {
namespace sqlite3 {

struct statement
{
    using native_type = struct sqlite3_stmt *;
    using result_type = debby::result<debby::backend::sqlite3::result>;

    struct rep_type
    {
        mutable native_type sth;
        bool cached;
    };

    static rep_type make (native_type sth, bool cached);

    template <typename T>
    static void bind_helper (rep_type * rep, std::string const & placeholder, T && value);

    template <typename T>
    static void bind (rep_type * rep, std::string const & placeholder, T && value)
    {
        auto v = to_storage(std::forward<T>(value));
        //bind_helper<T>(rep, placeholder, std::move(v));
        bind_helper(rep, placeholder, std::move(v));
    }
};

}}} // namespace debby::backend::sqlite3

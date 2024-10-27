////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "result.hpp"
#include "cast_traits.hpp"
#include "time_point_traits.hpp"
#include "uuid_traits.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/result.hpp"
#include "pfs/type_traits.hpp"
#include <string>
#include <vector>

struct pg_conn;

namespace debby {
namespace backend {
namespace psql {

struct statement
{
    using native_type = struct pg_conn *;
    using result_type = debby::result<debby::backend::psql::result>;

    struct rep_type
    {
        native_type dbh;
        std::string name;
        std::vector<std::string> param_transient_values;
        std::vector<char const *> param_values;
        std::vector<int> param_lengths;
        std::vector<int> param_formats;
    };

    static DEBBY__EXPORT rep_type make (native_type dbh, std::string const & name);

    template <typename T>
    static bool bind_helper (rep_type * rep, int index, T && value, error * perr);

    template <typename T>
    static bool bind_helper (rep_type * rep, std::string const & placeholder
        , T && value, error * perr);

    template <typename T>
    static bool bind (rep_type * rep, int index, T && value, error * perr)
    {
        auto v = to_storage(std::forward<T>(value));
        return bind_helper(rep, index, std::move(v), perr);
    }

    template <typename T>
    static bool bind (rep_type * rep, std::string const & placeholder
        , T && value, error * perr)
    {
        auto v = to_storage(std::forward<T>(value));
        return bind_helper(rep, placeholder, std::move(v), perr);
    }
};

}}} // namespace debby::backend::psql

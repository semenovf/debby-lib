////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "cast_traits.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/unified_value.hpp"
#include "pfs/optional.hpp"
#include <string>
#include <type_traits>
#include <unordered_map>

struct pg_result;

namespace debby {
namespace backend {
namespace psql {

#include "../assigner.hpp"

struct result
{
    using handle_type = struct pg_result *;
    using value_type = unified_value;

//     enum status {
//           INITIAL
//         , FAILURE
//         , DONE
//         , ROW
//     };

    struct rep_type
    {
        mutable handle_type sth;
        int row_count; // Total number of tuples
        int row_index;
//         status state; // {INITIAL};
//         int error_code; // {0};
//         mutable std::unordered_map<std::string, int> column_mapping;
    };

    static DEBBY__EXPORT rep_type make (handle_type sth);

    template <typename NativeType>
    static void assign (NativeType & target, value_type & v)
    {
        assigner<typename std::decay<NativeType>::type>{}(target, v);
    }
};

}}} // namespace debby::backend::psql

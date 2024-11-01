////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.26 Initial version.
//      2022.03.12 Refactored.
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

struct sqlite3_stmt;

namespace debby {
namespace backend {
namespace sqlite3 {

#include "../assigner.hpp"

struct result {
    using handle_type = struct sqlite3_stmt *;
    using value_type = unified_value;

    enum status {
          INITIAL
        , FAILURE
        , DONE
        , ROW
    };

    struct rep_type
    {
        mutable handle_type sth;
        status state; // {INITIAL};
        int error_code; // {0};
        mutable std::unordered_map<std::string, int> column_mapping;
    };

    static DEBBY__EXPORT rep_type make (handle_type sth, status state, int error_code);

    template <typename NativeType>
    static void assign (NativeType & target, value_type & v)
    {
        assigner<typename std::decay<NativeType>::type>{}(target, v);
    }
};

}}} // namespace debby::backend::sqlite3

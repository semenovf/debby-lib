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
#include "pfs/debby/result.hpp"

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
};

}}} // namespace debby::backend::sqlite3

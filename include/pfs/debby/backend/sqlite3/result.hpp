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
#include <string>
#include <unordered_map>

struct sqlite3_stmt;

namespace debby {
namespace backend {
namespace sqlite3 {

struct result {
    using handle_type = struct sqlite3_stmt *;

    enum status {
          INITIAL
        , ERROR
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

    static rep_type make (handle_type sth, status state, int error_code);
};

}}} // namespace debby::backend::sqlite3


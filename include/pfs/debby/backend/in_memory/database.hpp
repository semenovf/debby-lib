////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/unified_value.hpp"
#include <map>
#include <memory>
#include <mutex>

namespace debby {
namespace backend {
namespace in_memory {

struct database
{
    using key_type    = std::string;
    using value_type  = unified_value;
    using native_type = std::map<key_type, unified_value>;

    struct rep_type
    {
        std::unique_ptr<std::mutex> mtx;
        native_type dbh;
    };

    static DEBBY__EXPORT rep_type make_kv (error * perr = nullptr);
};

}}} // namespace debby::backend::in_memory

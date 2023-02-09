////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.09 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "associative_container.hpp"
#include "pfs/debby/unified_value.hpp"
#include <unordered_map>

namespace debby {
namespace backend {
namespace in_memory {

using unordered_map_st = database_st<std::unordered_map<std::string, unified_value>>;
using unordered_map_mt = database_mt<std::unordered_map<std::string, unified_value>>;

}}} // namespace debby::backend::in_memory



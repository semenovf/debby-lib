////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
//      2023.11.25 Using common implementation.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/time_point.hpp"
#include "affinity_traits.hpp"
#include "cast_traits.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

#include "../time_point_traits_common.hpp"

}}} // namespace debby::backend::sqlite3

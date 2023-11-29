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
#include "affinity_traits.hpp"
#include "cast_traits.hpp"
#include "pfs/universal_id.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

#include "../uuid_traits_common.hpp"

}}} // namespace debby::backend::sqlite3

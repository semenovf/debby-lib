////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2022.09.08 Initial version.
//      2023.11.25 Using common implementation.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "cast_traits.hpp"
#include "pfs/sha256.hpp"

namespace debby {
namespace backend {
namespace sqlite3 {

#include "../sha256_traits_common.hpp"

}}} // namespace debby::backend::sqlite3

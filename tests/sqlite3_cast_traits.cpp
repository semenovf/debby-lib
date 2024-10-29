////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
//      2023.11.26 2023.11.25 Using common implementation.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/string_view.hpp"
#include "pfs/debby/backend/sqlite3/cast_traits.hpp"
#include "pfs/debby/backend/sqlite3/time_point_traits.hpp"
#include "pfs/debby/backend/sqlite3/uuid_traits.hpp"
#include <cstdlib>

using namespace debby::backend::sqlite3;

#include "cast_traits_common.hpp"

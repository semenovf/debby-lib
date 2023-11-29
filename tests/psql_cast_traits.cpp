////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/string_view.hpp"
#include "pfs/debby/backend/psql/cast_traits.hpp"
#include "pfs/debby/backend/psql/time_point_traits.hpp"
#include "pfs/debby/backend/psql/uuid_traits.hpp"

using namespace debby::backend::psql;

#include "cast_traits_common.hpp"

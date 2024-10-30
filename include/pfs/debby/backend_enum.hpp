////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.10.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"

DEBBY__NAMESPACE_BEGIN

enum class backend_enum
{
      sqlite3 = 1
    , psql
};

DEBBY__NAMESPACE_END

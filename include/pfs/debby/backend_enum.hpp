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
      sqlite3 = 1      // R and K/V
    , psql             // R and K/V
    , map_st           // in-memory thread unsafe map, K/V only
    , map_mt           // in-memory thread safe map, K/V only
    , unordered_map_st // in-memory thread unsafe unordered_map, K/V only
    , unordered_map_mt // in-memory thread safe unordered_map, K/V only
    , lmdb             // KV only
    , mdbx             // KV only
    , rocksdb          // KV only
};

DEBBY__NAMESPACE_END

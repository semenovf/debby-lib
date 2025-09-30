////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2025.09.29 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "namespace.hpp"
#include "backend_enum.hpp"

DEBBY__NAMESPACE_BEGIN

template <backend_enum Backend, typename T>
struct column_type_affinity
{
    static char const * type ();
};

template <typename T>
struct value_type_affinity;

DEBBY__NAMESPACE_END

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.16 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "fixed_packer.hpp"
#include "debby/namespace.hpp"

DEBBY__NAMESPACE_BEGIN

template <typename T>
union fixed_packer
{
    T value;
    char bytes[sizeof(T)];
};

DEBBY__NAMESPACE_END
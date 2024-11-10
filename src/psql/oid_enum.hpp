////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "debby/namespace.hpp"
//
// This file contains enumerations represented the common Oid values from server/catalog/pg_type_d.h
//
DEBBY__NAMESPACE_BEGIN

namespace psql {

enum class oid_enum: unsigned int // See Oid definition
{
      boolean =  16 // BOOLOID
    , blob    =  17 // BYTEAOID
    , name    =  19 // NAMEOID
    , int16   =  21 // INT2OID
    , int32   =  23 // INT4OID
    , int64   =  20 // INT8OID
    , text    =  25 // TEXTOID
    , float32 = 700 // FLOAT4OID
    , float64 = 701 // FLOAT8OID
};

} // namespace psql

DEBBY__NAMESPACE_END

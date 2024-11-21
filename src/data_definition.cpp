////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "debby/data_definition.hpp"

DEBBY__NAMESPACE_BEGIN

// CREATE TABLE ...
// 1. [SQLite3](https://www.sqlite.org/lang_createtable.html)
// 2. [PostgreSQL](https://www.postgresql.org/docs/current/sql-createtable.html)

// 1. CREATE [opts1] TABLE [IF NOT EXISTS] [scheme-name.]table-name (column-def,... [table-constraint,...]) [table-options]
// 2. CREATE [opts1] TABLE [IF NOT EXISTS]

// 1. opts1 :=                  {TEMPORARY | TEMP}
// 2. opts1 := [GLOBAL | LOCAL] {TEMPORARY | TEMP} | UNLOGGED

// 1. column-def := column-name [type-name] [column-constraint,...]
// 2.

column::column (std::string && name, std::string && type)
    : _name(std::move(name))
    , _type(std::move(type))
{}

column & column::primary_key ()
{
    _flags.set(flag_bit::primary_flag);
    return *this;
}

column & column::unique ()
{
    _flags.set(flag_bit::unique_flag);
    return *this;
}

column & column::nullable ()
{
    _flags.set(flag_bit::nullable_flag);
    return *this;
}

DEBBY__NAMESPACE_END

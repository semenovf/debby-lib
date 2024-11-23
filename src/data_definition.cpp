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
// 2. CREATE [opts1] TABLE [IF NOT EXISTS]               table-name ({column-def | table-constraint | LIKE source-table [like-option ... ]},...])

// 1. opts1 :=                  {TEMPORARY | TEMP}
// 2. opts1 := [GLOBAL | LOCAL] {TEMPORARY | TEMP} | UNLOGGED

// 1. column-def := column-name [type-name]                                                                                                              [column-constraint,...]
// 2. column-def := column-name  type-name [STORAGE {PLAIN | EXTERNAL | EXTENDED | MAIN | DEFAULT}] [COMPRESSION compression_method] [COLLATE collation] [column_constraint,...]]

// 1. column-constraint := [CONSTRAINT name] PRIMARY KEY [{ASC | DESC}] [conflict-clause] [AUTOINCREMENT]
//                         [CONSTRAINT name] NOT NULL [conflict-clause]
//                         [CONSTRAINT name] UNIQUE [conflict-clause]
//                         [CONSTRAINT name] CHECK '(' expr ')'
//                         [CONSTRAINT name] DEFAULT { '(' expr ')' | literal-value | signed-number}
//                         [CONSTRAINT name] COLLATE collation-name
//                         [CONSTRAINT name] foreign-key-clause
//                         [CONSTRAINT name] [GENERATED ALWAYS] AS '(' expr ')' [{STORED | VIRTUAL}]
//
// 2. column-constraint := [CONSTRAINT name] PRIMARY KEY [index-parameters] [column-constraint-suffix]
//                         [CONSTRAINT name] NOT NULL [column-constraint-suffix]
//                         [CONSTRAINT name] NULL [column-constraint-suffix]
//                         [CONSTRAINT name] CHECK '(' expr ')' [NO INHERIT] [column-constraint-suffix]
//                         [CONSTRAINT name] DEFAULT expr [column-constraint-suffix]
//                         [CONSTRAINT name] GENERATED ALWAYS AS '(' expr ')' STORED [column-constraint-suffix]
//                         [CONSTRAINT name] GENERATED {ALWAYS | BY DEFAULT} AS IDENTITY ['(' sequence-options ')'] [column-constraint-suffix]
//                         [CONSTRAINT name] UNIQUE [NULLS [NOT] DISTINCT] [index-parameters] [column-constraint-suffix]
//                         [CONSTRAINT name] REFERENCES reftable ['(' refcolumn ')'] [{MATCH FULL | MATCH PARTIAL | MATCH SIMPLE}] [ON DELETE referential-action] [ON UPDATE referential-action] [column-constraint-suffix]
// 2. column-constraint-suffix := [{DEFERRABLE | NOT DEFERRABLE}] [{INITIALLY DEFERRED | INITIALLY IMMEDIATE}]


// 1. conflict-clause := ON CONFLICT { ROLLBACK | ABORT | FAIL | IGNORE | REPLACE }
// 2. -

column::column (std::string && name, std::string && type)
    : _name(std::move(name))
    , _type(std::move(type))
{}

column & column::primary_key (sort_order so, autoincrement autoinc)
{
    _flags.set(flag_bit::primary_flag);
    _sort_order = so;
    _autoinc = autoinc;
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

column & column::constraint (std::string text)
{
    _constraint = std::move(text);
    return *this;
}

DEBBY__NAMESPACE_END

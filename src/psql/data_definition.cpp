////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.23 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "debby/data_definition.hpp"
#include <pfs/universal_id.hpp>
#include <pfs/time_point.hpp>
#include <sstream>

DEBBY__NAMESPACE_BEGIN

// [Chapter 8. Data Types](https://www.postgresql.org/docs/current/datatype.html)

using data_definition_t = data_definition<backend_enum::psql>;
using table_t = table<backend_enum::psql>;

template <>
void column::build<backend_enum::psql> (std::ostream & out)
{
    out << _name << ' ' << _type;

    if (_flags.test(flag_bit::primary_flag)) {
        _flags.reset(flag_bit::nullable_flag);
        out << " PRIMARY KEY";
    }

    if (_flags.test(flag_bit::unique_flag)) {
        _flags.reset(flag_bit::nullable_flag);
        out << " UNIQUE";
    }

    if (!_flags.test(flag_bit::nullable_flag)) {
        out << " NOT NULL";
    }

    if (!_constraint.empty())
        out << ' ' << _constraint;
}

template <> char const * column_type_affinity<backend_enum::psql, bool>::type () {return "BOOLEAN";}
template <> char const * column_type_affinity<backend_enum::psql, char>::type () {return "SMALLINT";}
template <> char const * column_type_affinity<backend_enum::psql, signed char>::type () {return "SMALLINT";}
template <> char const * column_type_affinity<backend_enum::psql, unsigned char>::type () {return "SMALLINT";}
template <> char const * column_type_affinity<backend_enum::psql, short int>::type () {return "SMALLINT";}
template <> char const * column_type_affinity<backend_enum::psql, unsigned short int>::type () {return "SMALLINT";}
template <> char const * column_type_affinity<backend_enum::psql, int>::type () {return "INTEGER";}
template <> char const * column_type_affinity<backend_enum::psql, unsigned int>::type () {return "INTEGER";}
template <> char const * column_type_affinity<backend_enum::psql, long int>::type () {return "BIGINT";}
template <> char const * column_type_affinity<backend_enum::psql, unsigned long int>::type () {return "BIGINT";}
template <> char const * column_type_affinity<backend_enum::psql, long long int>::type () {return "BIGINT";}
template <> char const * column_type_affinity<backend_enum::psql, unsigned long long int>::type () {return "BIGINT";}
template <> char const * column_type_affinity<backend_enum::psql, float>::type () {return "REAL";}
template <> char const * column_type_affinity<backend_enum::psql, double>::type () {return "DOUBLE PRECISION";}
template <> char const * column_type_affinity<backend_enum::psql, std::string>::type () {return "TEXT";}
template <> char const * column_type_affinity<backend_enum::psql, blob_t>::type () {return "BYTEA";}

template <>
table_t::table (std::string && name)
    : _name(std::move(name))
{}

template <>
table_t & table_t::temporary ()
{
    _flags.set(flag_bit::temporary_flag);
    return *this;
}

template <>
void table_t::build (std::ostream & out)
{
    out << "CREATE";

    if (_flags.test(flag_bit::temporary_flag))
        out << " TEMPORARY";

    out << " TABLE IF NOT EXISTS \"" << _name << "\" (";

    char const * comma = ", ";
    char const * column_delim = "";

    for (auto & c: _columns) {
        out << column_delim;
        c.build<backend_enum::sqlite3>(out);
        column_delim = comma;
    }

    out << ')';
}

template <>
std::string table_t::build ()
{
    std::ostringstream out;
    build(out);
    return out.str();
}

template <>
table_t data_definition_t::create_table (std::string name)
{
    return table_t{std::move(name)};
}

DEBBY__NAMESPACE_END

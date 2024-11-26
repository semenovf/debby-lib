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

template <> template <> char const * table_t::column_type_affinity<bool>::value = "BOOLEAN";
template <> template <> char const * table_t::column_type_affinity<char>::value = "SMALLINT";
template <> template <> char const * table_t::column_type_affinity<signed char>::value = "SMALLINT";
template <> template <> char const * table_t::column_type_affinity<unsigned char>::value = "SMALLINT";
template <> template <> char const * table_t::column_type_affinity<short int>::value = "SMALLINT";
template <> template <> char const * table_t::column_type_affinity<unsigned short int>::value = "SMALLINT";
template <> template <> char const * table_t::column_type_affinity<int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<unsigned int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<long int>::value = "BIGINT";
template <> template <> char const * table_t::column_type_affinity<unsigned long int>::value = "BIGINT";
template <> template <> char const * table_t::column_type_affinity<long long int>::value = "BIGINT";
template <> template <> char const * table_t::column_type_affinity<unsigned long long int>::value = "BIGINT";
template <> template <> char const * table_t::column_type_affinity<float>::value = "REAL";
template <> template <> char const * table_t::column_type_affinity<double>::value = "DOUBLE PRECISION";
template <> template <> char const * table_t::column_type_affinity<std::string>::value = "TEXT";
template <> template <> char const * table_t::column_type_affinity<blob_t>::value = "BYTEA";
template <> template <> char const * table_t::column_type_affinity<pfs::universal_id>::value = "CHARACTER(26)";

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

    out << " TABLE IF NOT EXISTS " << _name << " (";

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

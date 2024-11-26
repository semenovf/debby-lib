////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "debby/data_definition.hpp"
#include <pfs/universal_id.hpp>
#include <sstream>

DEBBY__NAMESPACE_BEGIN

// [Datatypes In SQLite](https://www.sqlite.org/datatype3.html)

using data_definition_t = data_definition<backend_enum::sqlite3>;
using table_t = table<backend_enum::sqlite3>;
using index_t = index<backend_enum::sqlite3>;

template <>
void column::build<backend_enum::sqlite3> (std::ostream & out)
{
    out << _name << ' ' << _type;

    if (_flags.test(flag_bit::primary_flag)) {
        _flags.reset(flag_bit::nullable_flag);
        out << " PRIMARY KEY";

        switch (_sort_order) {
            case sort_order::asc:
                out << " ASC";
                break;
            case sort_order::desc:
                out << " DESC";
                break;
            default:
                break;
        }

        if (_autoinc == autoincrement::yes)
            out << " AUTOINCREMENT";
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

template <> template <> char const * table_t::column_type_affinity<bool>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<char>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<signed char>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<unsigned char>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<short int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<unsigned short int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<unsigned int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<long int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<unsigned long int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<long long int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<unsigned long long int>::value = "INTEGER";
template <> template <> char const * table_t::column_type_affinity<float>::value = "REAL";
template <> template <> char const * table_t::column_type_affinity<double>::value = "REAL";
template <> template <> char const * table_t::column_type_affinity<std::string>::value = "TEXT";
template <> template <> char const * table_t::column_type_affinity<blob_t>::value = "BLOB";
template <> template <> char const * table_t::column_type_affinity<pfs::universal_id>::value = "TEXT";

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
table_t & table_t::constraint (std::string text)
{
    _constraint = std::move(text);
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

    if (!_constraint.empty())
        out << ' ' << _constraint;
}

template <>
std::string table_t::build ()
{
    std::ostringstream out;
    build(out);
    return out.str();
}

template <>
index_t::index (std::string && name)
    : _name(std::move(name))
{}

template <>
void index_t::build (std::ostream & out)
{
    out << "CREATE";

    if (_unique)
        out << " UNIQUE";

    out << " INDEX IF NOT EXISTS "
        << '"' << _name << '"'
        << " ON " << '"' << _table << '"'
        << " (";

    char const * comma = ", ";
    char const * column_delim = "";

    for (auto & c: _columns) {
        out << column_delim << c;
        column_delim = comma;
    }

    out << ')';
}

template <>
std::string index_t::build ()
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

template <>
index_t data_definition_t::create_index (std::string name)
{
    return index_t{std::move(name)};
}

DEBBY__NAMESPACE_END

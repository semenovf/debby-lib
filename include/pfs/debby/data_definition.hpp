////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "backend_enum.hpp"
#include "exports.hpp"
#include "namespace.hpp"
#include <bitset>
#include <cstdint>
#include <ostream>
#include <vector>

DEBBY__NAMESPACE_BEGIN

using blob_t = void *;

enum class sort_order { none, asc, desc };
enum class autoincrement { no, yes };

class column
{
private:
    enum flag_bit: std::size_t
    {
          primary_flag
        , unique_flag
        , nullable_flag
        , max_flag = nullable_flag
    };

private:
    std::string _name;
    std::string _type;
    std::bitset<flag_bit::max_flag + 1> _flags;
    sort_order _sort_order {sort_order::none};
    autoincrement _autoinc {autoincrement::no};
    std::string _constraint;

public:
    DEBBY__EXPORT column (std::string && name, std::string && type);
    column (column const & other) = delete;
    column (column && other) = default;
    column & operator = (column const & other) = delete;
    column & operator = (column && other) = default;

public:
    /**
     * Adds PRIMARY KEY constraint for column
     *
     * @param so Sort order (sqlite3 only).
     * @param autoinc Autoincremented (sqlite3 only).
     */
    DEBBY__EXPORT column & primary_key (sort_order so = sort_order::none, autoincrement autoinc = autoincrement::no);

    DEBBY__EXPORT column & unique ();
    DEBBY__EXPORT column & nullable ();
    DEBBY__EXPORT column & constraint (std::string text);

    template <backend_enum Backend>
    DEBBY__EXPORT void build (std::ostream & out);
};

template <backend_enum Backend>
class table
{
public:
    enum flag_bit: std::size_t
    {
          temporary_flag
        , max_flag = temporary_flag
    };

private:
    std::string _name;
    std::vector<column> _columns;
    std::bitset<flag_bit::max_flag + 1> _flags;
    std::string _constraint;

public:
    DEBBY__EXPORT table (std::string && name);

    table (table const & other) = delete;
    table (table && other) = default;
    table & operator = (table const & other) = delete;
    table & operator = (table && other) = default;

public:
    DEBBY__EXPORT table & temporary ();
    DEBBY__EXPORT table & constraint (std::string text);

    template <typename T>
    inline column & add_column (std::string && name)
    {
        _columns.emplace_back(std::move(name), column_type_affinity<Backend, std::decay_t<T>>::type());
        return _columns.back();
    }

    DEBBY__EXPORT void build (std::ostream & out);
    DEBBY__EXPORT std::string build ();
};

template <backend_enum Backend>
class index
{
private:
    std::string _name;
    std::string _table;
    bool _unique {false};
    std::vector<std::string> _columns;

public:
    DEBBY__EXPORT index (std::string && name);

    index (index const & other) = delete;
    index (index && other) = default;
    index & operator = (index const & other) = delete;
    index & operator = (index && other) = default;

public:
    index & on (std::string table_name)
    {
        _table = std::move(table_name);
        return *this;
    }

    index & unique ()
    {
        _unique = true;
        return *this;
    }

    inline index & add_column (std::string && name)
    {
        _columns.emplace_back(std::move(name));
        return *this;
    }

    DEBBY__EXPORT void build (std::ostream & out);
    DEBBY__EXPORT std::string build ();
};

template <backend_enum Backend>
class data_definition
{
public:
    DEBBY__EXPORT data_definition ();
    DEBBY__EXPORT ~data_definition ();

    data_definition (data_definition const & other) = delete;
    data_definition (data_definition && other) noexcept = delete;
    data_definition & operator = (data_definition const & other) = delete;
    data_definition & operator = (data_definition && other) noexcept = delete;

public:
    static DEBBY__EXPORT table<Backend> create_table (std::string name);
    static DEBBY__EXPORT index<Backend> create_index (std::string name);
};

DEBBY__NAMESPACE_END

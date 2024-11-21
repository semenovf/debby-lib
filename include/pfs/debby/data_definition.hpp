////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "backend_enum.hpp"
// #include "error.hpp"
#include "exports.hpp"
#include "namespace.hpp"
#include <bitset>
#include <cstdint>
#include <ostream>
#include <vector>

DEBBY__NAMESPACE_BEGIN

using blob_t = void *;

class column
{
private:
    enum flag_bit: std::size_t
    {
          primary_flag
        , unique_flag
        , nullable_flag
    };

private:
    std::string _name;
    std::string _type;
    std::bitset<4> _flags;

public:
    DEBBY__EXPORT column (std::string && name, std::string && type);
    column (column const & other) = delete;
    column (column && other) = default;
    column & operator = (column const & other) = delete;
    column & operator = (column && other) = default;

public:
    // Set flag methods
    DEBBY__EXPORT column & primary_key ();
    DEBBY__EXPORT column & unique ();
    DEBBY__EXPORT column & nullable ();

    template <backend_enum Backend>
    DEBBY__EXPORT void build (std::ostream & out);
};

template <backend_enum Backend>
class table
{
public:
    template <typename T>
    struct column_type_affinity
    {
        static char const * value;
    };

private:
    std::string _name;
    std::vector<column> _columns;

public:
    DEBBY__EXPORT table (std::string && name);

    table (table const & other) = delete;
    table (table && other) = default;
    table & operator = (table const & other) = delete;
    table & operator = (table && other) = default;

public:
    template <typename T = blob_t>
    DEBBY__EXPORT column & add_column (std::string && name)
    {
        _columns.emplace_back(std::move(name), column_type_affinity<std::decay_t<T>>::value);
        return _columns.back();
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
    static DEBBY__EXPORT table<Backend> create_table (std::string && name);
};

DEBBY__NAMESPACE_END

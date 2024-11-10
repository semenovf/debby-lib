////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.02 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "debby/statement.hpp"
#include <algorithm>
#include <cstdint>
#include <type_traits>

extern "C" {
#include <libpq-fe.h>
}

DEBBY__NAMESPACE_BEGIN

using statement_t = statement<backend_enum::psql>;

template <>
class statement_t::impl
{
public:
    using native_type = struct pg_conn *;

    static constexpr int INC_SIZE = 8;

private:
    native_type _dbh {nullptr};
    std::string _name;
    std::vector<std::string> _param_transient_values;
    std::vector<char const *> _param_values;
    std::vector<int> _param_lengths;
    std::vector<int> _param_formats;

public:
    impl (native_type dbh, std::string const & name)
        : _dbh(dbh)
        , _name(name)
    {}

    impl (impl && other)
        : _dbh(other._dbh)
        , _name(std::move(other._name))
        , _param_transient_values(std::move(other._param_transient_values))
        , _param_values(std::move(other._param_values))
        , _param_lengths(std::move(other._param_lengths))
        , _param_formats(std::move(other._param_formats))
    {
        other._dbh = nullptr;
    }

    ~impl () = default;

private:
    void ensure_capacity (int index)
    {
        if (index >= _param_transient_values.size()) {
            std::size_t size = index + 1;
            auto reserve_size = (std::max)(size, _param_transient_values.size() + INC_SIZE);
            _param_transient_values.reserve(reserve_size);
            _param_values.reserve(reserve_size);
            _param_lengths.reserve(reserve_size);
            _param_formats.reserve(reserve_size);

            _param_transient_values.resize(size);
            _param_values.resize(size);
            _param_lengths.resize(size);
            _param_formats.resize(size);
        }
    }

public:
    // Bind value as text representation
    template <typename NumericType>
    typename std::enable_if<std::is_arithmetic<NumericType>::value, bool>::type
    bind_helper (int index, NumericType value)
    {
        ensure_capacity(index);
        _param_transient_values[index] = std::to_string(value);
        _param_values[index] = nullptr; // nullptr -> expected transient value
        _param_lengths[index] = _param_transient_values[index].size();
        _param_formats[index] = 0;
        return true;
    }

    bool bind_helper (int index, std::nullptr_t)
    {
        ensure_capacity(index);
        _param_values[index] = nullptr;
        _param_lengths[index] = 0;
        _param_formats[index] = 1;
        return true;
    }

    bool bind_helper (int index, std::string && value)
    {
        ensure_capacity(index);
        _param_transient_values[index] = std::move(value);
        _param_values[index] = nullptr; // nullptr -> expected transient value
        _param_lengths[index] = _param_transient_values[index].size();
        _param_formats[index] = 0;
        return true;
    }

    statement_t::result_type exec (error * perr);
};

DEBBY__NAMESPACE_END



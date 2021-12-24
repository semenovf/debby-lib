////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "result.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/basic_statement.hpp"
#include <functional>
#include <string>

struct sqlite3_stmt;

namespace debby {
namespace sqlite3 {

struct statement_traits
{
    using result_type = result;
};

class database;

class statement: public basic_statement<statement, statement_traits>
{
    friend class basic_statement<statement, statement_traits>;
    friend class database;

public:
    using native_type = struct sqlite3_stmt *;
    using result_type = statement_traits::result_type;

private:
    using base_class = basic_statement<statement, statement_traits>;

private:
    mutable native_type _sth {nullptr};
    bool                _cached {false};

private:
    statement (native_type sth, bool cached)
        : _sth(sth)
        , _cached(cached)
    {}

    std::string current_sql () const noexcept;

    inline bool is_prepared () const noexcept
    {
        return _sth != nullptr;
    }

    void clear () noexcept;
    result_type exec_impl (error * perr);
    int rows_affected_impl () const;

    bool bind_helper (std::string const & placeholder
        , std::function<int (int /*index*/)> && sqlite3_binder_func
        , error * perr);

    bool bind_impl (std::string const & placeholder, std::nullptr_t, error * perr);
    bool bind_impl (std::string const & placeholder, bool value, error * perr);
    bool bind_impl (std::string const & placeholder, std::int8_t value, error * perr);
    bool bind_impl (std::string const & placeholder, std::uint8_t value, error * perr);
    bool bind_impl (std::string const & placeholder, std::int16_t value, error * perr);
    bool bind_impl (std::string const & placeholder, std::uint16_t value, error * perr);
    bool bind_impl (std::string const & placeholder, std::int32_t value, error * perr);
    bool bind_impl (std::string const & placeholder, std::uint32_t value, error * perr);
    bool bind_impl (std::string const & placeholder, std::int64_t value, error * perr);
    bool bind_impl (std::string const & placeholder, std::uint64_t value, error * perr);
    bool bind_impl (std::string const & placeholder, float value, error * perr);
    bool bind_impl (std::string const & placeholder, double value, error * perr);
    bool bind_impl (std::string const & placeholder, char const * value
        , std::size_t len, bool static_value, error * perr);
    bool bind_impl (std::string const & placeholder
        , std::vector<std::uint8_t> const & value
        , bool static_value, error * perr);

    bool bind_impl (std::string const & placeholder
        , std::string const & value
        , bool static_value, error * perr)
    {
        return bind_impl(placeholder, value.data(), value.size(), static_value, perr);
    }

public:
    statement () = delete;

    ~statement ()
    {
        clear();
    }

    statement (statement const &) = delete;
    statement & operator = (statement const &) = delete;

    statement (statement && other)
    {
        *this = std::move(other);
    }

    statement & operator = (statement && other)
    {
        clear();
        _sth = other._sth;
        other._sth = nullptr;
        return *this;
    }
};

}} // namespace debby::sqlite3

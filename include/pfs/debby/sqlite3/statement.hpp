////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "result.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/basic_statement.hpp"
#include <functional>
#include <string>

struct sqlite3_stmt;

namespace pfs {
namespace debby {
namespace sqlite3 {

struct statement_traits
{
    using result_type = result;
};

PFS_DEBBY__EXPORT class statement: public basic_statement<statement, statement_traits>
{
    friend class basic_statement<statement, statement_traits>;

public:
    using native_type = struct sqlite3_stmt *;
    using result_type = statement_traits::result_type;

private:
    using base_class = basic_statement<statement, statement_traits>;

private:
    static std::string const ERROR_DOMAIN;

private:
    native_type _sth {nullptr};
    bool        _cached {false};

private:
    std::string current_sql () const noexcept;

    inline bool bool_cast_impl () const noexcept
    {
        return _sth != nullptr;
    }

    void clear_impl () noexcept;

    // throws sql_error
    result_type exec_impl ();

    // throws sql_error
    bool bind_helper (std::string const & placeholder
        , std::function<int (int /*index*/)> && bind_wrapper);

    bool bind_impl (std::string const & placeholder, std::nullptr_t);
    bool bind_impl (std::string const & placeholder, bool value);
    bool bind_impl (std::string const & placeholder, std::int8_t value);
    bool bind_impl (std::string const & placeholder, std::uint8_t value);
    bool bind_impl (std::string const & placeholder, std::int16_t value);
    bool bind_impl (std::string const & placeholder, std::uint16_t value);
    bool bind_impl (std::string const & placeholder, std::int32_t value);
    bool bind_impl (std::string const & placeholder, std::uint32_t value);
    bool bind_impl (std::string const & placeholder, std::int64_t value);
    bool bind_impl (std::string const & placeholder, std::uint64_t value);
    bool bind_impl (std::string const & placeholder, float value);
    bool bind_impl (std::string const & placeholder, double value);
    bool bind_impl (std::string const & placeholder, char const * value, std::size_t len);
    bool bind_impl (std::string const & placeholder, std::vector<std::uint8_t> const & value);

    inline bool bind_impl (std::string const & placeholder, std::string const & value)
    {
        return bind_impl(placeholder, value.data(), value.size());
    }

public:
    statement () = delete;

    statement (native_type sth, bool cached)
        : _sth(sth)
        , _cached(cached)
    {}

    ~statement ()
    {
        clear_impl();
    }

    statement (statement const &) = delete;
    statement & operator = (statement const &) = delete;

    statement (statement && other)
    {
        *this = std::move(other);
    }

    statement & operator = (statement && other)
    {
        clear_impl();
        _sth = other._sth;
        other._sth = nullptr;
        return *this;
    }
};

}}} // namespace pfs::debby::sqlite3

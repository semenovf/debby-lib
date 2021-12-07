////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "sqlite3.h"
#include "result.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/basic_statement.hpp"
#include <functional>
#include <string>

namespace pfs {
namespace debby {
namespace sqlite3 {

PFS_DEBBY__EXPORT class statement: public basic_statement<statement>
{
    friend class basic_statement<statement>;

public:
    using native_type = struct sqlite3_stmt *;

private:
    using base_class = basic_statement<statement>;

private:
    native_type _sth {nullptr};
    bool        _cached {false};
    std::string _last_error;

private:
    std::string last_error_impl () const noexcept
    {
        return _last_error;
    }

    void clear_impl ();
    result exec_impl ();

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
    bool bind_impl (std::string const & placeholder, std::string const & value);
    bool bind_impl (std::string const & placeholder, char const * value);
    bool bind_impl (std::string const & placeholder, std::vector<std::uint8_t> const & value);

public:
    statement () {}

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

    operator bool () const noexcept
    {
        return _sth != nullptr;
    }

    result exec ()
    {
        return exec_impl();
    }
};

}}} // namespace pfs::debby::sqlite3

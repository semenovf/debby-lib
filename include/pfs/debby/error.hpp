////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.14 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/expected.hpp"
#include "pfs/fmt.hpp"
#include <string>

namespace pfs {

// TODO
// Move into common-lib is expirience will be success

class exception
{
    std::string _domain;
    std::string _description;
    std::string _cause;

public:
    exception (std::string const & domain
        , std::string const & description
        , std::string const & cause)
        : _domain(domain)
        , _description(description)
        , _cause(cause)
    {}

    exception (std::string const & domain
        , std::string const & description)
        : exception(domain, description, std::string{})
    {}

    exception (std::string const & description)
        : exception(std::string{}, description, std::string{})
    {}

    std::string const & domain () const noexcept
    {
        return _domain;
    }

    std::string const & description () const noexcept
    {
        return _description;
    }

    std::string const & cause () const noexcept
    {
        return _cause;
    }

    virtual std::string what () const noexcept
    {
        std::string result;
        //std::string format = "[{}]: {} ({})";
        result.reserve(_domain.size() + _description.size() + _cause.size()
            + 10 /*format.size()*/); // <= no scrupulous accuracy needed

        if (!_domain.empty())
            result += '[' + _domain + "]";

        if (!_description.empty())
            result += std::string{(result.empty() ? "" : ": ")} + _description;

        if (!_cause.empty())
            result += std::string{(result.empty() ? "" : " ")} + '(' + _cause + ')';

        return result;
    }
};

class runtime_error: public exception
{
public:
    using exception::exception;
};

class logic_error: public exception
{
public:
    using exception::exception;
};

class bad_alloc: public exception
{
public:
    using exception::exception;
};

class bad_cast: public exception
{
public:
    using exception::exception;
};

class invalid_argument: public logic_error
{
public:
    using logic_error::logic_error;
};

class unexpected_error: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

} // namespace pfs

namespace pfs {
namespace debby {

class sql_error: public pfs::exception
{
public:
    sql_error (std::string const & domain
        , std::string const & description
        , std::string const & sql)
        : pfs::exception(domain, description, sql)
    {}
};

#define PFS_DEBBY_THROW(x) throw x

}} // namespace pfs::debby


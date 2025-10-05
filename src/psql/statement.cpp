////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
//      2024.10.29 V2 started.
//      2025.09.30 Changed bind implementation.
////////////////////////////////////////////////////////////////////////////////
#include "result_impl.hpp"
#include "statement_impl.hpp"
#include "utils.hpp"
#include <pfs/assert.hpp>
#include <pfs/i18n.hpp>

DEBBY__NAMESPACE_BEGIN

statement_t::result_type statement_t::impl::exec (error * perr)
{
    int result_in_text_format = 0;

    for (int i = 0; i < _param_transient_values.size(); i++) {
        if (_param_lengths[i] > 0) {
            if (_param_values[i] == nullptr) // nullptr - not a static value, expected transient value
                _param_values[i] = _param_transient_values[i].c_str();
        } else {
            _param_values[i] = nullptr;
        }
    }

    auto sth = PQexecPrepared(_dbh, _name.c_str()
        , _param_values.size()
        , _param_values.data()
        , _param_lengths.data()
        , _param_formats.data()
        , result_in_text_format);

    if (sth == nullptr) {
        pfs::throw_or(perr, make_error_code(errc::backend_error)
            , tr::f_("statement execution failure: {}", psql::build_errstr(_dbh)));

        return result_t{};
    }

    ExecStatusType status = PQresultStatus(sth);
    bool r = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);

    if (!r) {
        PQclear(sth);

        pfs::throw_or(perr, make_error_code(errc::backend_error)
            , tr::f_("statement execution failure: {}: {}", _name, psql::build_errstr(_dbh)));

        return result_t{};
    }

    return result_t{result_t::impl{sth}};
}

template <>
statement_t::statement () = default;

template <>
statement_t::statement (impl && d)
{
    _d = new impl(std::move(d));
}

template <>
statement_t::statement (statement && other) noexcept
{
    _d = other._d;
    other._d = nullptr;
}

template <>
statement_t::~statement ()
{
    if (_d != nullptr)
        delete _d;

    _d = nullptr;
}

template <>
statement_t & statement_t::operator = (statement && other)
{
    if (this != & other) {
        this->~statement();
        _d = other._d;
        other._d = nullptr;
    }

    return *this;
}

#define DEBBY__ARITHMETIC_BIND(t)                                   \
    template <>                                                     \
    template <>                                                     \
    bool statement_t::bind<t> (int index, t const & value, error *) \
    {                                                               \
        return _d->bind_arithmetic<t>(index, value);                \
    }

DEBBY__ARITHMETIC_BIND(bool)
DEBBY__ARITHMETIC_BIND(char)
DEBBY__ARITHMETIC_BIND(signed char)
DEBBY__ARITHMETIC_BIND(unsigned char)
DEBBY__ARITHMETIC_BIND(short)
DEBBY__ARITHMETIC_BIND(unsigned short)
DEBBY__ARITHMETIC_BIND(int)
DEBBY__ARITHMETIC_BIND(unsigned int)
DEBBY__ARITHMETIC_BIND(long)
DEBBY__ARITHMETIC_BIND(unsigned long)
DEBBY__ARITHMETIC_BIND(long long)
DEBBY__ARITHMETIC_BIND(unsigned long long)
DEBBY__ARITHMETIC_BIND(float)
DEBBY__ARITHMETIC_BIND(double)

template <>
template <>
bool statement_t::bind<std::string> (int index, std::string const & value, error *)
{
    return _d->bind_string(index, value.c_str(), value.size());
}

template <>
bool statement_t::bind (int index, std::nullptr_t, error *)
{
    return _d->bind_null(index, nullptr);
}

template <>
bool statement_t::bind (int index, char const * ptr, error *)
{
    return _d->bind_string(index, ptr, std::strlen(ptr));
}

template <>
bool statement_t::bind (int index, char const * ptr, std::size_t len, error *)
{
    return _d->bind_blob(index, ptr, len);
}

template <>
statement_t::result_type statement_t::exec (error * perr)
{
    return _d->exec(perr);
}

template <>
void statement_t::reset (error *)
{
    // Nothing to do
}

DEBBY__NAMESPACE_END

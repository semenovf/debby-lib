////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "utils.hpp"
#include "pfs/assert.hpp"
#include "pfs/endian.hpp"
#include "pfs/i18n.hpp"
#include "pfs/debby/statement.hpp"
#include "pfs/debby/backend/psql/result.hpp"
#include "pfs/debby/backend/psql/statement.hpp"
#include <algorithm>
#include <cstdio>

extern "C" {
#include <libpq-fe.h>
}

namespace debby {

static char const * NULL_HANDLER_TEXT = "uninitialized statement handler";

namespace backend {
namespace psql {

static constexpr int INCREMENT_PARAM_SIZE = 5;

void ensure_capacity (statement::rep_type * rep, int index)
{
    if (index >= rep->param_transient_values.size()) {
        std::size_t size = index + 1;
        auto reserve_size = (std::max)(size, rep->param_transient_values.size() + INCREMENT_PARAM_SIZE);
        rep->param_transient_values.reserve(reserve_size);
        rep->param_values.reserve(reserve_size);
        rep->param_lengths.reserve(reserve_size);
        rep->param_formats.reserve(reserve_size);

        rep->param_transient_values.resize(size);
        rep->param_values.resize(size);
        rep->param_lengths.resize(size);
        rep->param_formats.resize(size);
    }
}

statement::rep_type statement::make (native_type dbh, std::string const & name)
{
    rep_type rep;
    rep.dbh = dbh;
    rep.name = name;
    return rep;
}

static bool bind_helper_unsupported (error * perr)
{
    pfs::throw_or(perr, error {errc::unsupported, "binding with placeholder is not supported"});
    return false;
}

template <>
bool
statement::bind_helper (statement::rep_type * /*rep*/, std::string const & /*placeholder*/
    , bool && /*value*/, error * perr)
{
    return bind_helper_unsupported(perr);
}

#define BIND_INDEX_INT_BINARY_DEF(T)                                                 \
template <>                                                                          \
bool                                                                                 \
statement::bind_helper (statement::rep_type * rep                                    \
    , int index, T && value, error * /*perr*/)                                       \
{                                                                                    \
    ensure_capacity(rep, index);                                                     \
    auto transient_value = pfs::to_network_order(value);                             \
    rep->param_transient_values[index]                                               \
        = std::string(reinterpret_cast<char const *>(& transient_value), sizeof(T)); \
    rep->param_values[index] = nullptr;                                              \
    rep->param_lengths[index] = sizeof(T);                                           \
    rep->param_formats[index] = 1;                                                   \
    return true;                                                                     \
}

#define BIND_INDEX_INT_TEXT_DEF(T)                                                   \
template <>                                                                          \
bool                                                                                 \
statement::bind_helper (statement::rep_type * rep                                    \
    , int index, T && value, error * /*perr*/)                                       \
{                                                                                    \
    ensure_capacity(rep, index);                                                     \
    rep->param_transient_values[index] = std::to_string(value);                      \
    rep->param_values[index] = nullptr;                                              \
    rep->param_lengths[index] = rep->param_transient_values[index].size();           \
    rep->param_formats[index] = 0;                                                   \
    return true;                                                                     \
}

#define BIND_INDEX_INT_DEF(T) BIND_INDEX_INT_TEXT_DEF(T)

#define BIND_PLACEHOLDER_INT_DEF(T)                                      \
    template <>                                                          \
    bool                                                                 \
    statement::bind_helper (statement::rep_type * rep                    \
        , std::string const & placeholder, T && /*value*/, error * perr) \
    {                                                                    \
        return bind_helper_unsupported(perr);                            \
    }

BIND_INDEX_INT_DEF(char)
BIND_INDEX_INT_DEF(signed char)
BIND_INDEX_INT_DEF(unsigned char)
BIND_INDEX_INT_DEF(short)
BIND_INDEX_INT_DEF(unsigned short)
BIND_INDEX_INT_DEF(int)
BIND_INDEX_INT_DEF(unsigned int)
BIND_INDEX_INT_DEF(long)
BIND_INDEX_INT_DEF(unsigned long)

BIND_PLACEHOLDER_INT_DEF(char)
BIND_PLACEHOLDER_INT_DEF(signed char)
BIND_PLACEHOLDER_INT_DEF(unsigned char)
BIND_PLACEHOLDER_INT_DEF(short)
BIND_PLACEHOLDER_INT_DEF(unsigned short)
BIND_PLACEHOLDER_INT_DEF(int)
BIND_PLACEHOLDER_INT_DEF(unsigned int)
BIND_PLACEHOLDER_INT_DEF(long)
BIND_PLACEHOLDER_INT_DEF(unsigned long)

#if defined(_MSC_VER)
    BIND_INDEX_INT_DEF(__int64)
    BIND_INDEX_INT_DEF(unsigned __int64)
    BIND_PLACEHOLDER_INT_DEF(__int64)
    BIND_PLACEHOLDER_INT_DEF(unsigned __int64)
#endif

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, float && value, error *)
{
    ensure_capacity(rep, index);
    rep->param_transient_values[index] = std::to_string(value);
    rep->param_values[index] = nullptr; // nullptr -> expected transient value
    rep->param_lengths[index] = rep->param_transient_values[index].size();
    rep->param_formats[index] = 0;
    return true;
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, std::string const & placeholder
    , float && value, error * perr)
{
    return bind_helper_unsupported(perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, double && value, error *)
{
    ensure_capacity(rep, index);
    rep->param_transient_values[index] = std::to_string(value);
    rep->param_values[index] = nullptr; // nullptr -> expected transient value
    rep->param_lengths[index] = rep->param_transient_values[index].size();
    rep->param_formats[index] = 0;
    return true;
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, std::string const & placeholder
    , double && value, error * perr)
{
    return bind_helper_unsupported(perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, std::nullptr_t &&, error *)
{
    ensure_capacity(rep, index);
    rep->param_values[index] = nullptr;
    rep->param_lengths[index] = 0;
    rep->param_formats[index] = 1;
    return true;
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, std::string const & placeholder
    , std::nullptr_t &&, error * perr)
{
    return bind_helper_unsupported(perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, std::string && value, error * /*perr*/)
{
    ensure_capacity(rep, index);
    rep->param_transient_values[index] = std::move(value);
    rep->param_values[index] = nullptr; // nullptr -> expected transient value
    rep->param_lengths[index] = rep->param_transient_values[index].size();
    rep->param_formats[index] = 0;
    return true;
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, std::string const & placeholder
    , std::string && value, error * perr)
{
    return bind_helper_unsupported(perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, char const * && value, error * perr)
{
    return bind_helper(rep, index, std::string(value), perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, std::string const & placeholder
    , char const * && value, error * perr)
{
    return bind_helper_unsupported(perr);
}

template <>
bool
statement::bind_helper (statement::rep_type * rep, int index, bool && value, error * perr)
{
    return bind_helper(rep, index, std::string(value ? "1" : "0"), perr);
}

}} // namespace backend::psql

using BACKEND = backend::psql::statement;

template <>
statement<BACKEND>::statement ()
{
    _rep = BACKEND::make(nullptr, std::string{});
}

template <>
statement<BACKEND>::statement (rep_type && rep)
    : _rep(std::move(rep))
{}

template <>
statement<BACKEND>::statement (statement && other) noexcept
{
    _rep = std::move(other._rep);
    other._rep.dbh = nullptr;
}

template <>
statement<BACKEND>::~statement ()
{}

template <>
statement<BACKEND>::operator bool () const noexcept
{
    return _rep.dbh != nullptr;
}

template <>
statement<BACKEND>::result_type
statement<BACKEND>::exec (error * perr)
{
    PFS__ASSERT(_rep.dbh, NULL_HANDLER_TEXT);

    int result_in_binary_format = 1;

    for (int i = 0; i < _rep.param_transient_values.size(); i++) {
        if (_rep.param_lengths[i] > 0) {
            if (_rep.param_values[i] == nullptr) // nullptr - not a static value, expected transient value
                _rep.param_values[i] = _rep.param_transient_values[i].c_str();
        } else {
            _rep.param_values[i] = nullptr;
        }
    }

    auto sth = PQexecPrepared(_rep.dbh, _rep.name.c_str()
        , _rep.param_values.size()
        , _rep.param_values.data()
        , _rep.param_lengths.data()
        , _rep.param_formats.data()
        , result_in_binary_format);

    if (sth == nullptr) {
        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::f_("statement execution failure: {}", backend::psql::build_errstr(_rep.dbh))
        });

        result_type::make(nullptr);
    }

    ExecStatusType status = PQresultStatus(sth);
    bool r = (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK);

    if (!r) {
        PQclear(sth);

        pfs::throw_or(perr, error {
              errc::backend_error
            , tr::f_("statement execution failure: {}: {}", _rep.name
                , backend::psql::build_errstr(_rep.dbh))
        });

        result_type::make(nullptr);
    }

    return result_type::make(sth);
}

} // namespace debby

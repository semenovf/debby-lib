////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
//      2024.11.02 V2 started.
////////////////////////////////////////////////////////////////////////////////
#include "oid_enum.hpp"
#include "result_impl.hpp"
#include <pfs/i18n.hpp>
#include <pfs/integer.hpp>
//
DEBBY__NAMESPACE_BEGIN

template <>
result_t::result () = default;

template <>
result_t::result (impl && d)
{
    _d = new impl(std::move(d));
}

template <>
result_t::result (result && other) noexcept
{
    _d = other._d;
    other._d = nullptr;
}

template <>
result_t::~result ()
{
    if (_d != nullptr)
        delete _d;

    _d = nullptr;
}

template <>
int result_t::rows_affected () const
{
    char * str = PQcmdTuples(_d->sth);

    if (str == nullptr)
        return 0;

    if (str[0] == '\x0')
        return 0;

    std::error_code ec;
    auto n = pfs::to_integer<int>(str, str + std::strlen(str), 10, ec);

    if (ec) {
        throw error {
              pfs::errc::unexpected_error
            , tr::f_("unexpected string value returned by `PQcmdTuples`"
                " represented the number of affected rows: {}", str)
        };
    }

    return n;
}

template <>
bool result_t::has_more () const noexcept
{
    return _d->row_index < _d->row_count;
}

template <>
bool result_t::is_done () const noexcept
{
    return _d->row_count == _d->row_index;
}

template <>
bool result_t::is_error () const noexcept
{
    // TODO IMPLEMENT
    return false;
}

template <>
int result_t::column_count () const noexcept
{
    return _d->field_count;
}

template <>
std::string result_t::column_name (int column) const noexcept
{
    auto cname = PQfname(_d->sth, column);
    return cname == nullptr ? std::string{} : std::string{cname};
}

template <>
void result_t::next ()
{
    if (_d->row_index < _d->row_count) {
        ++_d->row_index;
    } else {
        throw std::overflow_error("result::next()");
    }
}

inline char * ensure_capacity (char * buffer, std::size_t size, std::size_t requred_size)
{
    if (size < requred_size) {
        buffer = new char[requred_size];
        return buffer;
    }

    return buffer;
}

template <>
std::pair<char *, std::size_t>
result_t::fetch (int column, char * buffer, std::size_t initial_size, error & err) const
{
    if (column < 0 || column >= _d->field_count) {
        err = error {
              errc::column_not_found
            , tr::f_("bad column: {}, expected greater or equal to 0 and"
                " less than {}", column, _d->field_count)
        };

        return std::make_pair(static_cast<char *>(nullptr), std::size_t{0});
    }

    auto is_null = PQgetisnull(_d->sth, _d->row_index, column) != 0;

    if (is_null)
        return std::make_pair(static_cast<char *>(nullptr), std::size_t{0});

    //auto column_type = static_cast<psql::oid_enum>(PQftype(_d->sth, column));
    auto raw_data = reinterpret_cast<char const *>(PQgetvalue(_d->sth, _d->row_index, column));
    int size = PQgetlength(_d->sth, _d->row_index, column);

    buffer = ensure_capacity(buffer, initial_size, size);
    std::memcpy(buffer, raw_data, size);
    return std::make_pair(buffer, size);
}

template <>
std::pair<char *, std::size_t>
result_t::fetch (std::string const & column_name, char * buffer, std::size_t initial_size, error & err) const
{
    int column = -1;

    for (int i = 0; i < _d->field_count; i++) {
        auto name = PQfname(_d->sth, i);

        if (column_name == name) {
            column = i;
            break;
        }
    }

    if (column < 0) {
        err = error {
            errc::column_not_found
            , tr::f_("bad column name: {}", column_name)
        };

        return std::make_pair(static_cast<char *>(nullptr), std::size_t{0});
    }

    return fetch(column, buffer, initial_size, err);
}

DEBBY__NAMESPACE_END

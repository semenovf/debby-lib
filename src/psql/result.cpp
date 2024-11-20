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
#include <pfs/endian.hpp>
#include <pfs/i18n.hpp>
#include <pfs/integer.hpp>
#include <pfs/real.hpp>
//
DEBBY__NAMESPACE_BEGIN

template <>
result_t::result () = default;

template <>
result_t::result (impl && d) noexcept
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
    char const * str = PQcmdTuples(_d->sth);

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
    return _d->column_count;
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

#define CHECK_COLUMN_INDEX_BOILERPLATE                                           \
    if (column < 0 || column >= _d->column_count) {                             \
        pfs::throw_or(perr, error {                                             \
              errc::column_not_found                                            \
            , tr::f_("bad column index: {}, expected greater or equal to 0 and" \
                " less than {}", column, _d->column_count)                      \
        });                                                                     \
        return pfs::nullopt;                                                    \
    }                                                                           \
    auto is_null = PQgetisnull(_d->sth, _d->row_index, column) != 0;            \
    if (is_null)                                                                \
        return pfs::nullopt;


#define UNSUITABLE_ERROR_BOILERPLATE \
    pfs::throw_or(perr, error {errc::bad_value, tr::f_("unsuitable column type at index {}", column)});

template <>
pfs::optional<std::int64_t> result_t::get_int64 (int column, error * perr)
{
    CHECK_COLUMN_INDEX_BOILERPLATE

    int size = PQgetlength(_d->sth, _d->row_index, column);

    if (size == 0)
        return pfs::nullopt;

    auto t = static_cast<psql::oid_enum>(PQftype(_d->sth, column));

    switch (t) {
        case psql::oid_enum::int16:
        case psql::oid_enum::int32:
        case psql::oid_enum::int64: {
            std::error_code ec;
            auto raw_data = PQgetvalue(_d->sth, _d->row_index, column);
            auto x = pfs::to_integer<std::int64_t>(raw_data, raw_data + size, ec);

            if (ec) {
                pfs::throw_or(perr, error {errc::bad_value, tr::f_("parse integer stored at column {} failure: {}"
                    , column, ec.message())});
                return pfs::nullopt;
            }

            return x;
        }

        case psql::oid_enum::boolean: {
            auto raw_data = PQgetvalue(_d->sth, _d->row_index, column);
            return raw_data[0] == 't' ? static_cast<std::int64_t>(true) : static_cast<std::int64_t>(false);
        }

        // Typically used by key/value database
        case psql::oid_enum::blob: {
            std::error_code ec;
            auto raw_data = PQgetvalue(_d->sth, _d->row_index, column);
            auto is_suitable = (size > 2 && size % 2 == 0 && raw_data[0] == '\\' && raw_data[1] == 'x');

            if (!is_suitable)
                break;

            auto x = pfs::to_integer<std::uint64_t>(raw_data + 2, raw_data + size, 16, ec);
            x = static_cast<std::int64_t>(pfs::to_native_order(x));

            if (ec) {
                pfs::throw_or(perr, error {errc::bad_value, tr::f_("parse integer stored at column {} failure: {}"
                    , column, ec.message())});
                return pfs::nullopt;
            }

            return x;
        }

        default:
            break;
    }

    UNSUITABLE_ERROR_BOILERPLATE
    return pfs::nullopt;
}

template <>
pfs::optional<double> result_t::get_double (int column, error * perr)
{
    CHECK_COLUMN_INDEX_BOILERPLATE

    int size = PQgetlength(_d->sth, _d->row_index, column);

    if (size == 0)
        return pfs::nullopt;

    auto t = static_cast<psql::oid_enum>(PQftype(_d->sth, column));

    switch (t) {
        case psql::oid_enum::float32:
        case psql::oid_enum::float64: {
            auto raw_data = PQgetvalue(_d->sth, _d->row_index, column);
            auto opt = pfs::to_real<double>(raw_data, raw_data + size, '.');
            return opt;
        }

        // Typically used by key/value database
        case psql::oid_enum::blob: {
            std::error_code ec;
            auto raw_data = PQgetvalue(_d->sth, _d->row_index, column);
            auto is_suitable = (size > 2 && size % 2 == 0 && raw_data[0] == '\\' && raw_data[1] == 'x');

            if (!is_suitable)
                break;

            static_assert(sizeof(double) == sizeof(std::int64_t), "Expected sizeof double equals to 64-bit integer");

            union {
                double f;
                std::uint64_t i;
            } n;

            n.i = pfs::to_integer<std::uint64_t>(raw_data + 2, raw_data + size, 16, ec);
            n.i = pfs::to_native_order(n.i);

            if (ec) {
                pfs::throw_or(perr, error {errc::bad_value, tr::f_("parse integer stored at column {} failure: {}"
                    , column, ec.message())});
                return pfs::nullopt;
            }

            return n.f;
        }

        default:
            break;
    }

    UNSUITABLE_ERROR_BOILERPLATE
    return pfs::nullopt;
}

inline int from_hex_char (char ch)
{
    if (ch >= '0' && ch <= '9')
        return int{ch - '0'};

    if (ch >= 'a' && ch <= 'f')
        return int{ch - 'a'} + 10;

    if (ch >= 'A' && ch <= 'F')
        return int{ch - 'A'} + 10;

    return -1;
}

template <>
pfs::optional<std::string> result_t::get_string (int column, error * perr)
{
    CHECK_COLUMN_INDEX_BOILERPLATE

    int size = PQgetlength(_d->sth, _d->row_index, column);

    if (size == 0)
        return pfs::nullopt;

    auto t = static_cast<psql::oid_enum>(PQftype(_d->sth, column));

    switch (t) {
        // Typically used by key/value database
        case psql::oid_enum::blob: {
            std::error_code ec;
            auto raw_data = PQgetvalue(_d->sth, _d->row_index, column);
            auto is_suitable = (size > 2 && size % 2 == 0 && raw_data[0] == '\\' && raw_data[1] == 'x');

            if (!is_suitable)
                break;

            bool success = true;
            std::string x;
            x.reserve(size - 2);

            for (int i = 2; i < size; i += 2) {
                auto a = from_hex_char(raw_data[i]);
                auto b = from_hex_char(raw_data[i + 1]);

                if (a < 0 || b < 0) {
                    success = false;
                    break;
                }

                int ch = a * 16 + b;
                x += static_cast<char>(ch);
            }

            if (success)
                return x;
        }

        default:
            break;
    }

    auto raw_data = reinterpret_cast<char const *>(PQgetvalue(_d->sth, _d->row_index, column));
    return std::string(raw_data, size);
}

template <>
pfs::optional<std::int64_t> result_t::get_int64 (std::string const & column_name, error * perr)
{
    auto index = _d->column_index(column_name);

    if (index < 0) {
        pfs::throw_or(perr, error {errc::column_not_found, tr::f_("bad column name: {}", column_name)});
        return 0;
    }

    return get_int64(index, perr);
}

template <>
pfs::optional<double> result_t::get_double (std::string const & column_name, error * perr)
{
    auto index = _d->column_index(column_name);

    if (index < 0) {
        pfs::throw_or(perr, error {errc::column_not_found, tr::f_("bad column name: {}", column_name)});
        return 0;
    }

    return get_double(index, perr);
}

template <>
pfs::optional<std::string> result_t::get_string (std::string const & column_name, error * perr)
{
    auto index = _d->column_index(column_name);

    if (index < 0) {
        pfs::throw_or(perr, error {errc::column_not_found, tr::f_("bad column name: {}", column_name)});
        return std::string{};
    }

    return get_string(index, perr);
}

DEBBY__NAMESPACE_END

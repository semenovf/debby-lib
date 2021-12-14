////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/optional.hpp"
#include "pfs/string_view.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/basic_result.hpp"
#include <string>
#include <unordered_map>

struct sqlite3_stmt;

namespace pfs {
namespace debby {
namespace sqlite3 {

class statement;

PFS_DEBBY__EXPORT class result: public basic_result<result>
{
    friend class basic_result<result>;
    friend class statement;

    using base_class = basic_result<result>;
    using handle_type = struct sqlite3_stmt *;

    enum status {
          INITIAL
        , ERROR
        , DONE
        , ROW
    };

public:
    using blob_type  = base_class::blob_type;
    using value_type = base_class::value_type;

private:
    static std::string const ERROR_DOMAIN;

private:
    mutable handle_type _sth {nullptr};
    status _state {INITIAL};
    std::unordered_map<string_view, int> _column_mapping;

private:
    result (handle_type sth, status state)
        : _sth(sth)
        , _state(state)
    {}

    std::string current_sql () const noexcept;

    bool has_more_impl () const noexcept
    {
        return _state == ROW;
    }

    bool is_done_impl () const noexcept
    {
        return _state == DONE;
    }

    bool is_error_impl () const noexcept
    {
        return _state == ERROR;
    }

    // throws sql_error
    void next_impl ();

    int column_count_impl () const noexcept;
    string_view column_name_impl (int column) const noexcept;

    // throws invalid_argument if column is out of bounds
    value_type fetch_impl (int column);

    // throws invalid_argument if column name is invalid
    value_type fetch_impl (string_view name);

    // throws invalid_argument if column name is invalid
    // throws bad_cast if unable to cast to requested type
    template <typename T>
    optional<T> get_impl (string_view name)
    {
        try {
            auto value = fetch_impl(name);

            // Ok, column contains null value
            if (holds_alternative<std::nullptr_t>(value))
                return nullopt;

            auto p = get_if_value_pointer<T>(& value);

            // Ok, goot casting
            if (p)
                return std::move(static_cast<T>(*p));

            // Bad casting
            PFS_DEBBY_THROW((bad_cast{
                  ERROR_DOMAIN
                , fmt::format("bad value cast for column: {}", name.to_string())
            }));
        } catch (...) {
            std::rethrow_exception(std::current_exception());
        }

        return nullopt;
    }

    // throws invalid_argument if column is out of bounds
    template <typename T>
    inline optional<T> get_impl (int column)
    {
        auto name = column_name(column);

        if (!name.empty())
            return this->get<T>(name);

        PFS_DEBBY_THROW((invalid_argument{
              ERROR_DOMAIN
            , fmt::format("bad column index: {}", column)
            , current_sql()
        }));

        return nullopt;
    }

public:
    ~result () {};
};

}}} // namespace pfs::debby::sqlite3

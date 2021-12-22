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
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/basic_result.hpp"
#include <string>
#include <unordered_map>

struct sqlite3_stmt;

namespace debby {
namespace sqlite3 {

class statement;

class result: public basic_result<result>
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
    using blob_type  = blob_t;
    using value_type = base_class::value_type;

private:
    mutable handle_type _sth {nullptr};
    status _state {INITIAL};
    std::unordered_map<pfs::string_view, int> _column_mapping;

private:
    result (handle_type sth, status state)
        : _sth(sth)
        , _state(state)
    {}

    std::string current_sql () const noexcept;

    inline bool is_ready () const noexcept
    {
        return _sth != nullptr;
    }

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

    int column_count_impl () const noexcept;
    pfs::string_view column_name_impl (int column) const noexcept;

    value_type fetch_impl (int column, error * perr);
    value_type fetch_impl (pfs::string_view name, error * perr);
    void next_impl (error * perr);

public:
    ~result () = default;
    result (result && other) = default;
    result & operator = (result && other) = default;
};

}} // namespace debby::sqlite3

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "sqlite3.h"
#include "pfs/optional.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/basic_result.hpp"
#include <string>

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
    handle_type _sth {nullptr};
    status _state {INITIAL};
    std::string _last_error;

private:
    result () {}

    result (handle_type sth, status state)
        : _sth(sth)
        , _state(state)
    {}

    std::string last_error_impl () const noexcept
    {
        return _last_error;
    }

    bool has_more_impl () const
    {
        return _state == ROW;
    }

    bool is_done_impl () const
    {
        return _state == DONE;
    }

    bool is_error_impl () const
    {
        return _state == ERROR;
    }

    void next_impl ();
    int column_count_impl () const;
    std::string column_name_impl (int column) const;
    optional<value_type> get_impl (int column);
    //optional<value_type> get_impl (std::string const & name);

public:
    ~result () {};
};

}}} // namespace pfs::debby::sqlite3


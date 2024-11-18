////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.10.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "sqlite3.h"
#include "debby/namespace.hpp"
#include "debby/result.hpp"
#include <unordered_map>

DEBBY__NAMESPACE_BEGIN

using result_t = result<backend_enum::sqlite3>;

template <>
class result_t::impl
{
public:
    using handle_type = struct sqlite3_stmt *;

    enum status {
          INITIAL
        , FAILURE
        , DONE
        , ROW
    };

public:
    mutable handle_type sth {nullptr};
    status state {INITIAL};
    int error_code {0};
    int column_count {0};
    mutable std::unordered_map<std::string, int> column_mapping;

private:
    bool _handle_owned {false};

public:
    impl (handle_type & h, status s, bool own_handle) noexcept
        : sth(h)
        , state(s)
        , _handle_owned(own_handle)
    {
        column_count =  sqlite3_column_count(sth);
        h = nullptr;
    }

    impl (impl && other) noexcept
    {
        sth = other.sth;
        state = other.state;
        error_code = other.error_code;
        column_count = other.column_count;
        column_mapping = std::move(other.column_mapping);
        _handle_owned = other._handle_owned;

        other.sth = nullptr;
        other._handle_owned = false;
    }

    ~impl ()
    {
        if (sth != nullptr && _handle_owned) {
            sqlite3_reset(sth);
            sqlite3_finalize(sth);
        }

        sth = nullptr;
    }

public:
    /**
     * @return Column index started from 0 and -1 if column not found
     */
    int column_index (std::string const & column_name)
    {
        if (column_mapping.empty()) {
            for (int i = 0; i < column_count; i++)
                column_mapping.insert({std::string{sqlite3_column_name(sth, i)}, i});
        }

        auto pos = column_mapping.find(column_name);

        if (pos == column_mapping.end())
            return -1;

        return pos->second;
    }
};

DEBBY__NAMESPACE_END

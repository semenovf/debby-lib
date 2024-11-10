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
    mutable std::unordered_map<std::string, int> column_mapping;

public:
    impl (handle_type h, status s) noexcept
        : sth(h)
        , state(s)
    {}

    impl (impl && other) noexcept
    {
        sth = other.sth;
        state = other.state;
        error_code = other.error_code;
        column_mapping = std::move(other.column_mapping);

        other.sth = nullptr;
    }
};

DEBBY__NAMESPACE_END


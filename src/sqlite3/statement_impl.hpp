////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.10.30 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "debby/statement.hpp"
#include "sqlite3.h"

DEBBY__NAMESPACE_BEGIN

using statement_t = statement<backend_enum::sqlite3>;

template <>
class statement_t::impl
{
public:
    using native_type = struct sqlite3_stmt *;

private:
    mutable native_type _sth {nullptr};
    bool _cached {false};

public:
    impl (native_type sth, bool cached) noexcept
        : _sth(sth)
        , _cached(cached)
    {}

    impl (impl && other) noexcept
    {
        _sth = other._sth;
        _cached = other._cached;
        other._sth = nullptr;
        other._cached = false;
    }

    ~impl ()
    {
        if (_sth != nullptr) {
            sqlite3_reset(_sth);

            if (!_cached)
                sqlite3_finalize(_sth);
        }

        _sth = nullptr;
    }

public:
    native_type native () const noexcept
    {
        return _sth;
    }

    void reset (error * perr);
    statement_t::result_type exec (bool move_handle_ownership, error * perr);
};

DEBBY__NAMESPACE_END

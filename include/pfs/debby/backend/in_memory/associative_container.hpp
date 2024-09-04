////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.08 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/unified_value.hpp"
#include <map>
#include <memory>
#include <mutex>

namespace debby {
namespace backend {
namespace in_memory {

// Thread unsafe backend
template <typename Container>
struct database_st
{
    using key_type    = typename Container::key_type;
    using value_type  = typename Container::mapped_type;
    using native_type = Container;

    struct lock_guard {
        lock_guard (bool) {}
    };

    struct rep_type
    {
        bool mtx;
        native_type dbh;
    };

    static DEBBY__EXPORT rep_type make_kv (error * perr = nullptr);

    static bool wipe (error * /*perr*/ = nullptr)
    {
        return true;
    }
};

// Thread safe backend
template <typename Container>
struct database_mt
{
    using key_type    = typename Container::key_type;
    using value_type  = typename Container::mapped_type;
    using native_type = Container;

    using lock_guard = std::lock_guard<std::mutex>;

    struct rep_type
    {
        mutable std::mutex mtx;
        native_type dbh;

        rep_type () = default;
        rep_type (rep_type && other) noexcept
            : mtx() // no need to move mutex
            , dbh(std::move(other.dbh)) {}
    };

    static DEBBY__EXPORT rep_type make_kv (error * perr = nullptr);

    static bool wipe (error * /*perr*/ = nullptr)
    {
        return true;
    }
};

}}} // namespace debby::backend::in_memory

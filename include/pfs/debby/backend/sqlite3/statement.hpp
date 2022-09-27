////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2021.12.18 Reimplemented with new error handling.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "result.hpp"
#include "cast_traits.hpp"
#include "time_point_traits.hpp"
#include "uuid_traits.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/result.hpp"
#include "pfs/type_traits.hpp"

struct sqlite3_stmt;

namespace debby {
namespace backend {
namespace sqlite3 {

struct statement
{
    using native_type = struct sqlite3_stmt *;
    using result_type = debby::result<debby::backend::sqlite3::result>;

    struct rep_type
    {
        mutable native_type sth;
        bool cached;
    };

    static DEBBY__EXPORT rep_type make (native_type sth, bool cached);

    template <typename T>
    static void bind_helper (rep_type * rep, int index, T && value);

    template <typename T>
    static void bind_helper (rep_type * rep, std::string const & placeholder, T && value);

    template <typename T>
    static void bind (rep_type * rep, int index, T && value)
    {
        auto v = to_storage(std::forward<T>(value));
        //bind_helper<T>(rep, index, std::move(v));
        bind_helper(rep, index, std::move(v));
    }

    template <typename T>
    static void bind (rep_type * rep, std::string const & placeholder, T && value)
    {
        auto v = to_storage(std::forward<T>(value));
        //bind_helper<T>(rep, placeholder, std::move(v));
        bind_helper(rep, placeholder, std::move(v));
    }
};

#if _MSC_VER
template <> DEBBY__EXPORT void statement::bind_helper<std::nullptr_t> (statement::rep_type * rep
    , int index, std::nullptr_t && value);
template <> DEBBY__EXPORT void statement::bind_helper<int> (statement::rep_type * rep
    , int index, int && value);
template <> DEBBY__EXPORT void statement::bind_helper<std::intmax_t> (statement::rep_type * rep
    , int index, std::intmax_t && value);
template <> DEBBY__EXPORT void statement::bind_helper<double> (statement::rep_type * rep
    , int index, double && value);
template <> DEBBY__EXPORT void statement::bind_helper<std::string> (statement::rep_type * rep
    , int index, std::string && value);
template <> DEBBY__EXPORT void statement::bind_helper<char const *> (statement::rep_type * rep
    , int index, char const * && value);

template <> DEBBY__EXPORT void statement::bind_helper<std::nullptr_t> (statement::rep_type * rep
    , std::string const & placeholder, std::nullptr_t && value);
template <> DEBBY__EXPORT void statement::bind_helper<int> (statement::rep_type * rep
    , std::string const & placeholder, int && value);
template <> DEBBY__EXPORT void statement::bind_helper<std::intmax_t> (statement::rep_type * rep
    , std::string const & placeholder, std::intmax_t && value);
template <> DEBBY__EXPORT void statement::bind_helper<double> (statement::rep_type * rep
    , std::string const & placeholder, double && value);
template <> DEBBY__EXPORT void statement::bind_helper<std::string> (statement::rep_type * rep
    , std::string const & placeholder, std::string && value);
template <> DEBBY__EXPORT void statement::bind_helper<char const *> (statement::rep_type * rep
    , std::string const & placeholder, char const * && value);
#endif

}}} // namespace debby::backend::sqlite3

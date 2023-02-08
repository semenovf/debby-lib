////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.26 Initial version.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "cast_traits.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/exports.hpp"
#include "pfs/debby/unified_value.hpp"
#include "pfs/optional.hpp"
#include <string>
#include <type_traits>
#include <unordered_map>

struct sqlite3_stmt;

namespace debby {
namespace backend {
namespace sqlite3 {

struct result {
    using handle_type = struct sqlite3_stmt *;
    using value_type = unified_value;

    enum status {
          INITIAL
        , FAILURE
        , DONE
        , ROW
    };

    struct rep_type
    {
        mutable handle_type sth;
        status state; // {INITIAL};
        int error_code; // {0};
        mutable std::unordered_map<std::string, int> column_mapping;
    };

    static DEBBY__EXPORT rep_type make (handle_type sth, status state, int error_code);

    template <typename NativeType>
    static void assign (NativeType & target, value_type & v)
    {
        using storage_type = typename affinity_traits<NativeType>::storage_type;

        auto ptr = get_if<storage_type>(& v);

        if (ptr) {
            target = to_native<NativeType>(*ptr);
        } else {
            if (std::is_same<typename std::decay<NativeType>::type, double>::value) {
                auto ptr = get_if<float>(& v);

                if (ptr) {
                    target = to_native<float>(*ptr);
                } else {
                    throw error { errc::bad_value, "unsuitable data requested" };
                }
            } else if (std::is_same<typename std::decay<NativeType>::type, float>::value) {
                auto ptr = get_if<double>(& v);

                if (ptr) {
                    target = to_native<double>(*ptr);
                } else {
                    throw error { errc::bad_value, "unsuitable data requested" };
                }
            } else {
                throw error { errc::bad_value, "unsuitable data requested" };
            }
        }
    }

    template <typename NativeType>
    static void assign (pfs::optional<NativeType> & target, value_type & v)
    {
        using storage_type = typename affinity_traits<pfs::optional<NativeType>>::storage_type;

        auto ptr = get_if<storage_type>(& v);

        if (ptr) {
            target = to_native<NativeType>(*ptr);
        } else {
            target = pfs::nullopt;
        }
    }
};

}}} // namespace debby::backend::sqlite3


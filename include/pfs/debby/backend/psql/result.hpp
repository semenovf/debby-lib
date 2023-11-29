////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.25 Initial version.
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

struct pg_result;

namespace debby {
namespace backend {
namespace psql {

// template <typename NativeType>
// struct assigner
// {
//     using value_type = unified_value;
//
//     void operator () (NativeType & target, value_type & v)
//     {
//         using storage_type = typename affinity_traits<NativeType>::storage_type;
//
//         auto ptr = get_if<storage_type>(& v);
//
//         if (ptr) {
//             target = to_native<NativeType>(*ptr);
//         } else {
//             throw error { errc::bad_value, "unsuitable data requested" };
//         }
//     }
// };
//
// template <>
// struct assigner<float>
// {
//     using value_type = unified_value;
//
//     void operator () (float & target, value_type & v)
//     {
//         using storage_type = typename affinity_traits<float>::storage_type;
//
//         auto ptr = get_if<storage_type>(& v);
//
//         if (ptr) {
//             target = to_native<float>(*ptr);
//         } else {
//             auto ptr = get_if<double>(& v);
//
//             if (ptr) {
//                 target = static_cast<float>(to_native<double>(*ptr));
//             } else {
//                 throw error { errc::bad_value, "unsuitable data requested" };
//             }
//         }
//     }
// };
//
//
// template <>
// struct assigner<double>
// {
//     using value_type = unified_value;
//
//     void operator () (double & target, value_type & v)
//     {
//         using storage_type = typename affinity_traits<double>::storage_type;
//
//         auto ptr = get_if<storage_type>(& v);
//
//         if (ptr) {
//             target = to_native<double>(*ptr);
//         } else {
//             auto ptr = get_if<float>(& v);
//
//             if (ptr) {
//                 target = to_native<float>(*ptr);
//             } else {
//                 throw error { errc::bad_value, "unsuitable data requested" };
//             }
//         }
//     }
// };
//
// template <typename NativeType>
// struct assigner<pfs::optional<NativeType>>
// {
//     using value_type = unified_value;
//
//     void operator () (pfs::optional<NativeType> & target, value_type & v)
//     {
//         using storage_type = typename affinity_traits<pfs::optional<NativeType>>::storage_type;
//
//         auto ptr = get_if<storage_type>(& v);
//
//         if (ptr) {
//             target = to_native<NativeType>(*ptr);
//         } else {
//             target = pfs::nullopt;
//         }
//     }
// };

struct result
{
    using handle_type = struct pg_result *;
    using value_type = unified_value;

//     enum status {
//           INITIAL
//         , FAILURE
//         , DONE
//         , ROW
//     };
//
    struct rep_type
    {
        mutable handle_type sth;
        int row_count; // Total number of tuples
        int row_index;
//         status state; // {INITIAL};
//         int error_code; // {0};
//         mutable std::unordered_map<std::string, int> column_mapping;
    };

    static DEBBY__EXPORT rep_type make (handle_type sth);

//     template <typename NativeType>
//     static void assign (NativeType & target, value_type & v)
//     {
//         assigner<typename std::decay<NativeType>::type>{}(target, v);
//     }
};

}}} // namespace debby::backend::psql

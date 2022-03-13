////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "affinity_traits.hpp"
#include "cast_traits.hpp"
#include "result.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/debby/result.hpp"
#include "pfs/debby/unified_value.hpp"
#include "pfs/fmt.hpp"
#include "pfs/optional.hpp"
#include <functional>
#include <string>

namespace debby {
namespace backend {
namespace sqlite3 {

class input_record
{
    using value_type = unified_value;
    using result_type = debby::result<result>;

public:
    class assign_wrapper
    {
        friend class input_record;

    private:
        result_type * _res {nullptr};
        std::string _column_name;

    public:
        template <typename NativeType>
        bool to (NativeType * target) noexcept
        {
            using storage_type = typename affinity_traits<NativeType>::storage_type;

            auto ptr = _res->template get<storage_type *>(_column_name);

            if (ptr) {
                auto opt = to_native<NativeType>(*ptr);

                if (opt.has_value()) {
                    *target = *opt;
                    return true;
                }
            }

            return false;
        }

        template <typename NativeType>
        bool to (pfs::optional<NativeType> * target) noexcept
        {
            using storage_type = typename affinity_traits<pfs::optional<NativeType>>::storage_type;

            auto ptr = _res->template get<storage_type *>(_column_name);

            if (ptr) {
                auto opt = to_native<NativeType>(*ptr);

                if (opt.has_value()) {
                    *target = *opt;
                    return true;
                }
            } else {
                *target = pfs::nullopt;
                return true;
            }

            return false;
        }

        template <typename NativeType>
        inline bool to (NativeType & target) noexcept
        {
            return to<NativeType>(& target);
        }

        template <typename NativeType>
        inline bool to (pfs::optional<NativeType> & target) noexcept
        {
            return to<NativeType>(& target);
        }

        template <typename NativeType>
        inline bool operator >> (NativeType * target) noexcept
        {
            return to<NativeType>(target);
        }

        template <typename NativeType>
        inline bool operator >> (pfs::optional<NativeType> * target) noexcept
        {
            return to<NativeType>(target);
        }

        template <typename NativeType>
        inline bool operator >> (NativeType & target) noexcept
        {
            return to<NativeType>(target);
        }

        template <typename NativeType>
        inline bool operator >> (pfs::optional<NativeType> & target) noexcept
        {
            return to<NativeType>(target);
        }
    };

private:
    result_type * _res {nullptr};

public:
    input_record (result_type * res)
        : _res(res)
    {}

    inline assign_wrapper assign (std::string const & column_name) noexcept
    {
        assign_wrapper aw;
        aw._res = _res;
        aw._column_name = column_name;
        return aw;
    }

    inline assign_wrapper operator [] (std::string const & column_name) noexcept
    {
        assign_wrapper aw;
        aw._res = _res;
        aw._column_name = column_name;
        return aw;
    }
};

}}} // namespace debby::backend::sqlite3

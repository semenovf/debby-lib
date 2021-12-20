////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "result.hpp"
#include "affinity_traits.hpp"
#include "cast_traits.hpp"
#include "pfs/debby/error.hpp"
#include "pfs/fmt.hpp"
#include "pfs/string_view.hpp"
#include <functional>
#include <string>

namespace debby {
namespace sqlite3 {

class input_record
{
    using value_type = result::value_type;

public:
    class assign_wrapper
    {
        friend class input_record;

    private:
        result * _res {nullptr};
        pfs::string_view _column_name;

    public:
        template <typename NativeType>
        bool to (NativeType * target) noexcept
        {
            using storage_type = typename debby::sqlite3::affinity_traits<NativeType>::storage_type;

            error err;
            auto opt_value = _res->template get<storage_type>(_column_name, & err);

            if (opt_value.has_value()) {
                auto opt = to_native<NativeType>(*opt_value);

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
            using storage_type = typename debby::sqlite3::affinity_traits<pfs::optional<NativeType>>::storage_type;

            error err;
            auto opt_value = _res->template get<storage_type>(_column_name, & err);

            if (opt_value.has_value()) {
                auto opt = to_native<NativeType>(*opt_value);

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
    result * _res {nullptr};

public:
    input_record (result & res)
        : _res(& res)
    {}

    inline assign_wrapper assign (pfs::string_view column_name) noexcept
    {
        assign_wrapper aw;
        aw._res = _res;
        aw._column_name = column_name;
        return aw;
    }

    inline assign_wrapper operator [] (pfs::string_view column_name) noexcept
    {
        assign_wrapper aw;
        aw._res = _res;
        aw._column_name = column_name;
        return aw;
    }
};

}} // namespace debby::sqlite3

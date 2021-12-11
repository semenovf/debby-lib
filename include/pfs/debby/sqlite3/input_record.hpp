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
#include "pfs/fmt.hpp"
#include "pfs/string_view.hpp"
#include <functional>
#include <string>

namespace pfs {
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
        string_view _column_name;

    public:
        template <typename NativeType>
        bool to (NativeType * target)
        {
            using storage_type = typename debby::sqlite3::affinity_traits<NativeType>::storage_type;

            auto expected_value = _res->template get<storage_type>(_column_name);
            bool success = !!expected_value;

            if (success) {
                auto opt = to_native<NativeType>(*expected_value);

                if (opt.has_value())
                    *target = *opt;
                else
                    success = false;
            }

            return success;
        }

        template <typename NativeType>
        bool to (optional<NativeType> * target)
        {
            using storage_type = typename debby::sqlite3::affinity_traits<optional<NativeType>>::storage_type;

            auto expected_value = _res->template get<storage_type>(_column_name);
            bool success = !!expected_value;

            if (success) {
                auto opt = debby::sqlite3::to_native<optional<NativeType>>(*expected_value);

                if (opt.has_value())
                    *target = *opt;
                else
                    success = false;
            } else {
                // `null` value at column
                if (!expected_value.error()) {
                    *target = nullopt;
                    success = true;
                }
            }

            return success;
        }

        template <typename NativeType>
        inline bool to (NativeType & target)
        {
            return to<NativeType>(& target);
        }

        template <typename NativeType>
        inline bool to (optional<NativeType> & target)
        {
            return to<optional<NativeType>>(& target);
        }

        template <typename NativeType>
        inline bool operator >> (NativeType * target)
        {
            return to<NativeType>(target);
        }

        template <typename NativeType>
        inline bool operator >> (optional<NativeType> * target)
        {
            return to<NativeType>(target);
        }

        template <typename NativeType>
        inline bool operator >> (NativeType & target)
        {
            return to<NativeType>(target);
        }

        template <typename NativeType>
        inline bool operator >> (optional<NativeType> & target)
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

    inline assign_wrapper assign (string_view column_name)
    {
        assign_wrapper aw;
        aw._res = _res;
        aw._column_name = column_name;
        return aw;
    }

    inline assign_wrapper operator [] (string_view column_name)
    {
        assign_wrapper aw;
        aw._res = _res;
        aw._column_name = column_name;
        return aw;
    }
};

}}} // namespace pfs::debby::sqlite3

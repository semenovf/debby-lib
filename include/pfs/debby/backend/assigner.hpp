////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.12.02 Initial version.
////////////////////////////////////////////////////////////////////////////////
//
// !!! DO NOT USE DIRECTLY
//

template <typename NativeType>
struct assigner
{
    using value_type = unified_value;

    void operator () (NativeType & target, value_type & v)
    {
        using storage_type = typename affinity_traits<NativeType>::storage_type;

        auto ptr = get_if<storage_type>(& v);

        if (ptr) {
            target = to_native<NativeType>(*ptr);
        } else {
            throw error { errc::bad_value, "unsuitable data requested" };
        }
    }
};

template <>
struct assigner<float>
{
    using value_type = unified_value;

    void operator () (float & target, value_type & v)
    {
        using storage_type = typename affinity_traits<float>::storage_type;

        auto ptr = get_if<storage_type>(& v);

        if (ptr) {
            target = to_native<float>(*ptr);
        } else {
            auto ptr = get_if<double>(& v);

            if (ptr) {
                target = static_cast<float>(to_native<double>(*ptr));
            } else {
                throw error { errc::bad_value, "unsuitable data requested" };
            }
        }
    }
};


template <>
struct assigner<double>
{
    using value_type = unified_value;

    void operator () (double & target, value_type & v)
    {
        using storage_type = typename affinity_traits<double>::storage_type;

        auto ptr = get_if<storage_type>(& v);

        if (ptr) {
            target = to_native<double>(*ptr);
        } else {
            auto ptr = get_if<float>(& v);

            if (ptr) {
                target = to_native<float>(*ptr);
            } else {
                throw error { errc::bad_value, "unsuitable data requested" };
            }
        }
    }
};

template <typename NativeType>
struct assigner<pfs::optional<NativeType>>
{
    using value_type = unified_value;

    void operator () (pfs::optional<NativeType> & target, value_type & v)
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

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
//      2023.11.25 Based on sqlite3/uuid_traits.hpp.
////////////////////////////////////////////////////////////////////////////////
//
// !!! DO NOT USE DIRECTLY
//

template <> struct affinity_traits<pfs::universal_id> : text_affinity_traits {};
template <> struct affinity_traits<pfs::universal_id &> : text_affinity_traits {};
template <> struct affinity_traits<pfs::universal_id const> : text_affinity_traits {};
template <> struct affinity_traits<pfs::universal_id const &> : text_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, pfs::universal_id>::value, void>::type>
{
    using storage_type = typename affinity_traits<pfs::universal_id>::storage_type;

    static storage_type to_storage (pfs::universal_id const & value)
    {
        return to_string(value);
    }

    static pfs::universal_id to_native (storage_type const & value)
    {
        return pfs::from_string<pfs::universal_id>(value);
    }
};

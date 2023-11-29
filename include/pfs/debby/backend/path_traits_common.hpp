////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
//      2023.11.25 Based on sqlite3/path_traits.hpp.
////////////////////////////////////////////////////////////////////////////////
//
// !!! DO NOT USE DIRECTLY
//

template <> struct affinity_traits<pfs::filesystem::path> : text_affinity_traits {};
template <> struct affinity_traits<pfs::filesystem::path &> : text_affinity_traits {};
template <> struct affinity_traits<pfs::filesystem::path const> : text_affinity_traits {};
template <> struct affinity_traits<pfs::filesystem::path const &> : text_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, pfs::filesystem::path>::value, void>::type>
{
    using storage_type = typename affinity_traits<pfs::filesystem::path>::storage_type;

    static storage_type to_storage (pfs::filesystem::path const & value)
    {
        return pfs::filesystem::utf8_encode(value);
    }

    static pfs::filesystem::path to_native (storage_type const & value)
    {
        return pfs::filesystem::utf8_decode(value);
    }
};

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2022.09.08 Initial version.
//      2023.11.25 Based on sqlite3/sha256_traits.hpp.
////////////////////////////////////////////////////////////////////////////////
//
// !!! DO NOT USE DIRECTLY
//

template <> struct affinity_traits<pfs::crypto::sha256_digest> : text_affinity_traits {};
template <> struct affinity_traits<pfs::crypto::sha256_digest &> : text_affinity_traits {};
template <> struct affinity_traits<pfs::crypto::sha256_digest const> : text_affinity_traits {};
template <> struct affinity_traits<pfs::crypto::sha256_digest const &> : text_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, pfs::crypto::sha256_digest>::value, void>::type>
{
    using storage_type = typename affinity_traits<pfs::crypto::sha256_digest>::storage_type;

    static storage_type to_storage (pfs::crypto::sha256_digest const & value)
    {
        return to_string(value);
    }

    static pfs::crypto::sha256_digest to_native (storage_type const & value)
    {
        return value.empty()
            ? pfs::crypto::sha256_digest{}
            : pfs::crypto::sha256_digest{value};
    }
};

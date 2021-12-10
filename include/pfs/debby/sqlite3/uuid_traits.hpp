////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [chat-lib](https://github.com/semenovf/chat-lib) library.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "pfs/uuid.hpp"
#include "affinity_traits.hpp"
#include "cast_traits.hpp"

namespace pfs {
namespace debby {
namespace sqlite3 {

template <> struct affinity_traits<uuid_t> : text_affinity_traits {};
template <> struct affinity_traits<uuid_t &> : text_affinity_traits {};
template <> struct affinity_traits<uuid_t const> : text_affinity_traits {};
template <> struct affinity_traits<uuid_t const &> : text_affinity_traits {};

template <typename NativeType>
struct cast_traits<NativeType, typename std::enable_if<
       std::is_same<pfs::remove_cvref_t<NativeType>, uuid_t>::value, void>::type>
{
    using storage_type = typename affinity_traits<uuid_t>::storage_type;

    static storage_type to_storage (uuid_t const & value)
    {
        return std::to_string(value);
    }

    static optional<uuid_t> to_native (storage_type const & value)
    {
        return from_string<uuid_t>(value);
    }
};

}}} // namespace pfs::debby::sqlite3

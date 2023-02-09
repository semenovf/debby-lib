////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.02.09 Initial version.
////////////////////////////////////////////////////////////////////////////////
#include "pfs/debby/backend/in_memory/unordered_map.hpp"

#define BACKEND debby::backend::in_memory::unordered_map_st
#include "associative_container_impl.hpp"

#undef BACKEND
#define BACKEND debby::backend::in_memory::unordered_map_mt
#include "associative_container_impl.hpp"

namespace debby {
namespace backend {
namespace in_memory {

template <>
unordered_map_st::rep_type unordered_map_st::make_kv (error * /*perr*/)
{
    return rep_type{true, native_type{}};
}

template <>
unordered_map_mt::rep_type unordered_map_mt::make_kv (error * /*perr*/)
{
    //return rep_type{pfs::make_unique<std::mutex>(), native_type{}};
    return rep_type{};
}

}}} // namespace debby::backend::in_memory

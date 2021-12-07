////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "basic_database.hpp"
#include <string>
#include <vector>

namespace pfs {
namespace debby {

template <typename Impl>
class keyvalue_database : public basic_database<Impl>
{};

}} // namespace pfs::debby

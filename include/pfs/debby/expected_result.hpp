////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.16 Initial version.
////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <system_error>

#if !PFS_DEBBY__EXCEPTIONS_ENABLED
#include "pfs/expected.hpp"
#endif

namespace pfs {
namespace debby {

#if PFS_DEBBY__EXCEPTIONS_ENABLED
template <typename T>
using expected_result = T;
#else
template <typename T>
using expected_result = expected<T,std::error_code>;
#endif

}} // namespace pfs::debby

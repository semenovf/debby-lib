////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.08.14 Initial version.
//      2021.11.17 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef PFS_DEBBY__STATIC
#   ifndef PFS_DEBBY__EXPORT
#       if _MSC_VER
#           if defined(PFS_DEBBY__EXPORTS)
#               define PFS_DEBBY__EXPORT __declspec(dllexport)
#           else
#               define PFS_DEBBY__EXPORT __declspec(dllimport)
#           endif
#       else
#           define PFS_DEBBY__EXPORT
#       endif
#   endif
#else
#   define PFS_DEBBY__EXPORT
#endif // !PFS_DEBBY__STATIC

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.08.14 Initial version.
//      2021.11.17 Refactored.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#ifndef DEBBY__STATIC
#   ifndef DEBBY__EXPORT
#       if _MSC_VER
#           if defined(DEBBY__EXPORTS)
#               define DEBBY__EXPORT __declspec(dllexport)
#           else
#               define DEBBY__EXPORT __declspec(dllimport)
#           endif
#       else
#           define DEBBY__EXPORT
#       endif
#   endif
#else
#   define DEBBY__EXPORT
#endif // !DEBBY__STATIC

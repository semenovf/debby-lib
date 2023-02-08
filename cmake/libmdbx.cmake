################################################################################
# Copyright (c) 2021-2023 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#      2023.02.06 Initial version.
################################################################################
cmake_minimum_required (VERSION 3.5)

if (NOT DEBBY__LIBMDBX_ROOT)
    set(DEBBY__LIBMDBX_ROOT ${CMAKE_CURRENT_LIST_DIR}/3rdparty/libmdbx)
endif()

# By default builds static `libmdbx` library if MDBX_BUILD_SHARED_LIBRARY
# not set to TRUE. But force this behaviour.
set(MDBX_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Enable/disable build shared `libmdbx` library")
set(MDBX_BUILD_TOOLS OFF CACHE BOOL "Disable build `libmdbx` tools")

add_subdirectory(${DEBBY__LIBMDBX_ROOT} libmdbx EXCLUDE_FROM_ALL)

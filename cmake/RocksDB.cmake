################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
#
# Changelog:
#      2021.11.17 Initial version.
#      2021.11.29 Renamed variables.
#      2021.12.09 Became the part of debby-lib and refactored totally.
################################################################################
cmake_minimum_required (VERSION 3.5)

#
# https://github.com/facebook/rocksdb/blob/main/INSTALL.md
#
set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "Disable build RocksDB as shared")
set(WITH_GFLAGS OFF CACHE BOOL "Disable 'gflags' dependency for RocksDB")
set(WITH_TESTS OFF CACHE BOOL "Disable build tests for RocksDB")
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "Disable build benchmarks for RocksDB")
set(WITH_CORE_TOOLS OFF CACHE BOOL "Disable build core tools for RocksDB")
set(WITH_TOOLS OFF CACHE BOOL "Disable build tools for RocksDB")
#set(FAIL_ON_WARNINGS OFF CACHE BOOL "Disable process warnings as errors for RocksDB")

if (NOT DEBBY__ROCKSDB_ROOT)
    set(DEBBY__ROCKSDB_ROOT ${CMAKE_CURRENT_LIST_DIR}/3rdparty/rocksdb)
endif()

add_subdirectory(${DEBBY__ROCKSDB_ROOT} rocksdb)
target_include_directories(rocksdb PUBLIC
    $<BUILD_INTERFACE:${DEBBY__ROCKSDB_ROOT}/include>)
    # $<INSTALL_INTERFACE:include/mylib>  # <prefix>/include/mylib

if (CMAKE_COMPILER_IS_GNUCXX)
    # Disable error for g++ 11.2.0 (RocksDB v6.25.3)
    # error: ‘hostname_buf’ may be used uninitialized [-Werror=maybe-uninitialized]
    target_compile_options(rocksdb PRIVATE "-Wno-maybe-uninitialized")

    # For link custom shared libraries with RocksDB static library
    target_compile_options(rocksdb PRIVATE "-fPIC")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # error: comparison of integers of different signs: 'long long' and
    # 'unsigned long long' [-Werror,-Wsign-compare]
    target_compile_options(rocksdb PRIVATE "-Wno-sign-compare")
endif()

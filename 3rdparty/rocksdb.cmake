################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#      2021.11.17 Initial version.
#      2021.11.29 Renamed variables.
#      2021.12.09 Became the part of debby-lib and refactored totally.
################################################################################
cmake_minimum_required (VERSION 3.5)

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/rocksdb/CMakeLists.txt)
    message(STATUS "RocksDB found at: ${CMAKE_CURRENT_LIST_DIR}/rocksdb" )
    #
    # https://github.com/facebook/rocksdb/blob/main/INSTALL.md
    #
    set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "Disable build RocksDB as shared")

    # ATTENTION! ROCKSDB_LITE causes a segmentation fault while open the database.
    # See `make_kv()` in `src/rocksdb/database.cpp`.
    #set(ROCKSDB_LITE ON CACHE BOOL "Build lite version for RocksDB (see ROCKSDB_LITE.md for details)")

    set(WITH_GFLAGS OFF CACHE BOOL "Disable 'gflags' dependency for RocksDB")
    set(WITH_TESTS OFF CACHE BOOL "Disable build tests for RocksDB")
    set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "Disable build benchmarks for RocksDB")
    set(WITH_CORE_TOOLS OFF CACHE BOOL "Disable build core tools for RocksDB")
    set(WITH_TOOLS OFF CACHE BOOL "Disable build tools for RocksDB")

    # This option fixes crashes on Windows similar to https://github.com/facebook/rocksdb/issues/9573
    set(PORTABLE ON CACHE BOOL "Build a portable binary")

    #set(FAIL_ON_WARNINGS OFF CACHE BOOL "Disable process warnings as errors for RocksDB")

    set(_rocksdb_root ${CMAKE_CURRENT_LIST_DIR}/rocksdb)

    add_subdirectory(${_rocksdb_root} rocksdb EXCLUDE_FROM_ALL)
    target_include_directories(rocksdb PUBLIC $<BUILD_INTERFACE:${_rocksdb_root}/include>)

    if (CMAKE_COMPILER_IS_GNUCXX)
        # Disable error for g++ 11.2.0 (RocksDB v6.25.3)
        # error: ‘hostname_buf’ may be used uninitialized [-Werror=maybe-uninitialized]
        target_compile_options(rocksdb PRIVATE
            "-Wno-maybe-uninitialized"
            "-Wno-redundant-move")

        # For link custom shared libraries with RocksDB static library
        target_compile_options(rocksdb PRIVATE "-fPIC")
    endif()

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(rocksdb PRIVATE "-Wno-sign-compare")
        target_compile_options(rocksdb PRIVATE "-Wno-deprecated-copy")
        #target_compile_options(rocksdb PRIVATE "-Wno-unused-but-set-variable")
    endif()
else()
    message(STATUS "No RocksDB found at: ${CMAKE_CURRENT_LIST_DIR}/3rdparty/rocksdb, so disabled" )
endif()

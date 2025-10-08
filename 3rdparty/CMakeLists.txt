################################################################################
# Copyright (c) 2025 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#       2025.08.01 Initial version
################################################################################
include(FetchContent)

if (DEBBY__ENABLE_ROCKSDB_CXX11)
    if (DEBBY__DISABLE_FETCH_CONTENT AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/rocksdb/.git)
        include(${CMAKE_CURRENT_LIST_DIR}/rocksdb.cmake)
    else()
        set(FETCHCONTENT_UPDATES_DISCONNECTED_ROCKSDB ON)
        message(STATUS "Fetching RocksDB ...")
        FetchContent_Declare(rocksdb
            GIT_REPOSITORY https://github.com/facebook/rocksdb.git
            GIT_TAG v6.29.5  # Last version compatible with C++11
            GIT_SHALLOW 1
            PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace
                "${CMAKE_CURRENT_LIST_DIR}/rocksdb-v6.29.5.patch"

            SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/rocksdb)
#            SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/rocksdb)

        if (NOT rocksdb_POPULATED)
            FetchContent_Populate(rocksdb)
        endif()
        message(STATUS "Fetching RocksDB complete")

        include(${CMAKE_CURRENT_LIST_DIR}/rocksdb.cmake)
    endif()
endif(DEBBY__ENABLE_ROCKSDB_CXX11)

if (DEBBY__ENABLE_MDBX)
    set(MDBX_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Enable/disable build shared `libmdbx` library")
    set(MDBX_BUILD_TOOLS OFF CACHE BOOL "Disable build `libmdbx` tools")
    set(MDBX_BUILD_CXX OFF CACHE BOOL "Disable build `libmdbx` with C++ support")
    set(MDBX_BUILD_TIMESTAMP "unknown" CACHE STRING "")

    if (DEBBY__DISABLE_FETCH_CONTENT AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/libmdbx/.git)
        add_subdirectory(libmdbx EXCLUDE_FROM_ALL)
    else()
        set(FETCHCONTENT_UPDATES_DISCONNECTED_LIBMDBX ON)
        message(STATUS "Fetching libmdbx ...")
        FetchContent_Declare(libmdbx
            GIT_REPOSITORY https://github.com/erthink/libmdbx.git
            GIT_TAG "v0.13.7"
            SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libmdbx)
        FetchContent_MakeAvailable(libmdbx)
        message(STATUS "Fetching libmdbx complete")
    endif()

    set_target_properties(mdbx-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif(DEBBY__ENABLE_MDBX)

if (DEBBY__ENABLE_LMDB)
    # Official repository
    # https://git.openldap.org/openldap/openldap.git

    if (DEBBY__DISABLE_FETCH_CONTENT AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/lmdb/.git)
        # Nothing to do
    else()
        set(FETCHCONTENT_UPDATES_DISCONNECTED_LMDB ON)
        message(STATUS "Fetching lmdb ...")
        FetchContent_Declare(lmdb
            GIT_REPOSITORY https://github.com/LMDB/lmdb.git
            GIT_TAG "LMDB_0.9.31"
            SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lmdb)

        FetchContent_GetProperties(lmdb)

        if (NOT lmdb_POPULATED)
            FetchContent_Populate(lmdb)
        endif()
        message(STATUS "Fetching lmdb complete")
    endif()

    add_library(lmdb-static STATIC
        ${CMAKE_CURRENT_LIST_DIR}/lmdb/libraries/liblmdb/mdb.c
        ${CMAKE_CURRENT_LIST_DIR}/lmdb/libraries/liblmdb/midl.c)
    target_include_directories(lmdb-static PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lmdb/libraries/liblmdb)
    set_target_properties(lmdb-static PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if (DEBBY__ENABLE_PSQL)
    if (DEBBY__DISABLE_FETCH_CONTENT AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/postgres/.git)
        # Nothing to do
    else()
        set(FETCHCONTENT_UPDATES_DISCONNECTED_POSTGRES ON)
        message(STATUS "Fetching postgres ...")

        FetchContent_Declare(postgres
            GIT_REPOSITORY https://github.com/postgres/postgres.git
            GIT_TAG REL_16_1
            GIT_SHALLOW 1
            SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/postgres)

        FetchContent_GetProperties(postgres)

        if (NOT postgres_POPULATED)
            FetchContent_Populate(postgres)
        endif()

        message(STATUS "Fetching postgres complete")
    endif()

    include(${CMAKE_CURRENT_LIST_DIR}/postgres.cmake)
endif()

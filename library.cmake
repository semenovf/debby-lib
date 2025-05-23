################################################################################
# Copyright (c) 2019-2022 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#       2021.11.24 Initial version.
#       2024.10.27 Removed `portable_target` dependency.
#       2024.11.12 Min CMake version is 3.15.
#       2024.11.13 Min CMake version is 3.19 (CMakePresets).
################################################################################
cmake_minimum_required (VERSION 3.19)
project(debby LANGUAGES CXX C)

if (DEBBY__BUILD_SHARED)
    add_library(debby SHARED)
    target_compile_definitions(debby PRIVATE DEBBY__EXPORTS)
else()
    add_library(debby STATIC)
    target_compile_definitions(debby PRIVATE DEBBY__STATIC)
endif()

add_library(pfs::debby ALIAS debby)

if (MSVC)
    target_compile_definitions(debby PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

list(APPEND _debby__sources
    ${CMAKE_CURRENT_LIST_DIR}/src/data_definition.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp)

if (DEBBY__ENABLE_MAP OR DEBBY__ENABLE_UNORDERED_MAP)
    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/in_memory/keyvalue_database.cpp)

    if (DEBBY__ENABLE_MAP)
        list(APPEND _debby__definitions "DEBBY__MAP_ENABLED=1")
    endif()

    if (DEBBY__ENABLE_UNORDERED_MAP)
        list(APPEND _debby__definitions "DEBBY__UNORDERED_MAP_ENABLED=1")
    endif()
endif()

if (DEBBY__ENABLE_SQLITE3)
    list(APPEND _debby__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/sqlite3.c
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/data_definition.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/relational_database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/keyvalue_database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/result.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/statement.cpp)

    list(APPEND _debby__definitions "DEBBY__SQLITE3_ENABLED=1")

    # Enable full-text search (FTS) functionality in sqlite3
    if (DEBBY__ENABLE_SQLITE_FTS5)
        target_compile_definitions(debby PRIVATE "SQLITE_ENABLE_FTS5=1")
    endif()
endif()

if (NOT TARGET pfs::common)
    set(FETCHCONTENT_UPDATES_DISCONNECTED_COMMON ON)
    message(STATUS "Fetching common ...")
    include(FetchContent)
    FetchContent_Declare(common
        GIT_REPOSITORY https://github.com/semenovf/common-lib.git
        GIT_TAG master
        GIT_SHALLOW 1
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/2ndparty/common
        SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/2ndparty/common)
    FetchContent_MakeAvailable(common)
    message(STATUS "Fetching common complete")
endif()

if (DEBBY__ENABLE_ROCKSDB)
    set(FETCHCONTENT_UPDATES_DISCONNECTED_ROCKSDB ON)
    message(STATUS "Fetching RocksDB ...")
    include(FetchContent)
    FetchContent_Declare(rocksdb
        GIT_REPOSITORY https://github.com/facebook/rocksdb.git
        GIT_TAG v6.29.5  # Last version compatible with C++11
        GIT_SHALLOW 1
        PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace
            "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rocksdb-v6.29.5.patch"

        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rocksdb
        SUBBUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/rocksdb)

    if (NOT rocksdb_POPULATED)
        FetchContent_Populate(rocksdb)
    endif()
    message(STATUS "Fetching RocksDB complete")

    include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/rocksdb.cmake)

    if (TARGET rocksdb)
        list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/rocksdb/keyvalue_database.cpp)
        list(APPEND _debby__definitions "DEBBY__ROCKSDB_ENABLED=1")

        target_link_libraries(debby PRIVATE rocksdb)
    else()
        message(WARNING "No RocksDB target found, may be need to download it (run ${CMAKE_CURRENT_LIST_DIR}/3rdparty/download.sh)")
    endif()
endif(DEBBY__ENABLE_ROCKSDB)

if (DEBBY__ENABLE_MDBX)
    set(MDBX_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Enable/disable build shared `libmdbx` library")
    set(MDBX_BUILD_TOOLS OFF CACHE BOOL "Disable build `libmdbx` tools")
    set(MDBX_BUILD_CXX OFF CACHE BOOL "Disable build `libmdbx` with C++ support")

    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/libmdbx/lib EXCLUDE_FROM_ALL)
    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/libmdbx/keyvalue_database.cpp)
    list(APPEND _debby__definitions "DEBBY__MDBX_ENABLED=1")
    target_link_libraries(debby PRIVATE mdbx-static)
endif(DEBBY__ENABLE_MDBX)

if (DEBBY__ENABLE_LMDB)
    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/lmdb/keyvalue_database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/lmdb/lib/mdb.c
        ${CMAKE_CURRENT_LIST_DIR}/src/lmdb/lib/midl.c)
    list(APPEND _debby__definitions "DEBBY__LMDB_ENABLED=1")
endif(DEBBY__ENABLE_LMDB)

if (DEBBY__ENABLE_PSQL)
    set(FETCHCONTENT_UPDATES_DISCONNECTED_PSQL ON)
    message(STATUS "Fetching postgres ...")
    include(FetchContent)
    FetchContent_Declare(postgres
        GIT_REPOSITORY https://github.com/postgres/postgres.git
        GIT_TAG REL_16_1
        GIT_SHALLOW 1
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/postgres)

    if (NOT psql_POPULATED)
        FetchContent_Populate(postgres)
    endif()
    message(STATUS "Fetching postgresql complete")

    set(_postgres_dir ${CMAKE_CURRENT_LIST_DIR}/3rdparty/postgres)

    if (EXISTS ${_postgres_dir} AND EXISTS ${_postgres_dir}/README)
        list(APPEND _debby__definitions "DEBBY__PSQL_ENABLED=1")
        list(APPEND _debby__include_dirs
            $<TARGET_PROPERTY:pgcommon,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:pgport,INTERFACE_INCLUDE_DIRECTORIES>
            $<TARGET_PROPERTY:pq-static,INTERFACE_INCLUDE_DIRECTORIES>)

        list(APPEND _debby__private_libs pq-static pgport pgcommon)

        include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/postgres.cmake)

        list(APPEND _debby__sources
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/data_definition.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/keyvalue_database.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/relational_database.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/result.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/statement.cpp)
    else()
        message(WARNING "PostgreSQL source directory not found: ${_postgres_dir}")
        set(DEBBY__ENABLE_PSQL FALSE)
    endif()
endif()

target_sources(debby PRIVATE ${_debby__sources})
target_compile_definitions(debby PUBLIC ${_debby__definitions})
target_include_directories(debby
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs)
target_link_libraries(debby PUBLIC pfs::common PRIVATE ${_debby__private_libs})

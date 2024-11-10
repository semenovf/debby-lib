################################################################################
# Copyright (c) 2019-2022 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#       2021.11.24 Initial version.
#       2024.10.27 Removed `portable_target` dependency.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(debby LANGUAGES CXX C)

option(DEBBY__BUILD_SHARED "Enable build shared library" OFF)
option(DEBBY__ENABLE_SQLITE3 "Enable `Sqlite3` backend" ON)
option(DEBBY__ENABLE_SQLITE_FTS5 "Enable support for `FTS5` in `Sqlite3` backend" OFF)
option(DEBBY__ENABLE_ROCKSDB "Enable `RocksDb` backend" OFF)
option(DEBBY__ENABLE_MDBX "Enable `libmdbx` backend" ON)
option(DEBBY__ENABLE_LMDB "Enable `LMDB` backend" ON)
option(DEBBY__ENABLE_PSQL  "Enable `PostgreSQL` front-end backend" OFF)
option(DEBBY__ENABLE_MAP  "Enable `in-memory` map backend" ON)
option(DEBBY__ENABLE_UNORDERED_MAP  "Enable `in-memory` unordered map backend" ON)

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

list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp)

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
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/database.cpp
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
    include(${CMAKE_CURRENT_LIST_DIR}/2ndparty/common/library.cmake)
endif()

if (DEBBY__ENABLE_ROCKSDB)
    if (NOT DEBBY__ROCKSDB_ROOT)
        set(DEBBY__ROCKSDB_ROOT "${CMAKE_CURRENT_LIST_DIR}/3rdparty/rocksdb" CACHE INTERNAL "")
    endif()

    message(STATUS "RocksDB root: [${DEBBY__ROCKSDB_ROOT}]")

    include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/rocksdb.cmake)

    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/rocksdb/database.cpp)
    list(APPEND _debby__definitions "DEBBY__ROCKSDB_ENABLED=1")

    target_link_libraries(debby PRIVATE rocksdb)
endif(DEBBY__ENABLE_ROCKSDB)

if (DEBBY__ENABLE_MDBX)
    set(MDBX_BUILD_SHARED_LIBRARY OFF CACHE BOOL "Enable/disable build shared `libmdbx` library")
    set(MDBX_BUILD_TOOLS OFF CACHE BOOL "Disable build `libmdbx` tools")
    set(MDBX_BUILD_CXX OFF CACHE BOOL "Disable build `libmdbx` with C++ support")

    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/src/libmdbx/lib EXCLUDE_FROM_ALL)
    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/libmdbx/keyvalue_database.cpp)
    list(APPEND _debby__definitions "DEBBY__MDBX_ENABLED=1")
    target_link_libraries(debby PRIVATE mdbx-static)
endif(DEBBY__ENABLE_LIBMDBX)

if (DEBBY__ENABLE_LMDB)
    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/lmdb/keyvalue_database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/lmdb/lib/mdb.c
        ${CMAKE_CURRENT_LIST_DIR}/src/lmdb/lib/midl.c)
    list(APPEND _debby__definitions "DEBBY__LMDB_ENABLED=1")
endif(DEBBY__ENABLE_LMDB)

if (DEBBY__ENABLE_PSQL)
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
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/database.cpp
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

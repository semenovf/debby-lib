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
target_link_libraries(debby PUBLIC pfs::common)

if (MSVC)
    target_compile_definitions(debby PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_sources(debby PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src/data_definition.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp)

if (DEBBY__ENABLE_MAP OR DEBBY__ENABLE_UNORDERED_MAP)
    target_sources(debby PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/in_memory/keyvalue_database.cpp)

    if (DEBBY__ENABLE_MAP)
        target_compile_definitions(debby PUBLIC "DEBBY__MAP_ENABLED=1")
    endif()

    if (DEBBY__ENABLE_UNORDERED_MAP)
        target_compile_definitions(debby PUBLIC "DEBBY__UNORDERED_MAP_ENABLED=1")
    endif()
endif()

if (DEBBY__ENABLE_SQLITE3)
    target_sources(debby PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/sqlite3.c
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/data_definition.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/relational_database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/keyvalue_database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/result.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/statement.cpp)

    target_compile_definitions(debby PUBLIC "DEBBY__SQLITE3_ENABLED=1")

    # Enable full-text search (FTS) functionality in sqlite3
    if (DEBBY__ENABLE_SQLITE_FTS5)
        target_compile_definitions(debby PRIVATE "SQLITE_ENABLE_FTS5=1")
    endif()
endif()

if (DEBBY__ENABLE_ROCKSDB)
    if (TARGET rocksdb)
        target_sources(debby PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/rocksdb/keyvalue_database.cpp)
        target_compile_definitions(debby PUBLIC "DEBBY__ROCKSDB_ENABLED=1")

        target_link_libraries(debby PRIVATE rocksdb)
    else()
        message(WARNING "No RocksDB target found, RocksDB support disabled")
    endif()
endif(DEBBY__ENABLE_ROCKSDB)

if (DEBBY__ENABLE_MDBX)
    if (TARGET mdbx-static)
        target_sources(debby PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/libmdbx/keyvalue_database.cpp)
        target_compile_definitions(debby PUBLIC "DEBBY__MDBX_ENABLED=1")
        target_link_libraries(debby PRIVATE mdbx-static)
    else()
        message(WARNING "No MDBX target found, MDBX support disabled")
    endif()
endif(DEBBY__ENABLE_MDBX)

if (DEBBY__ENABLE_LMDB)
    if (TARGET lmdb-static)
        target_sources(debby PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/lmdb/keyvalue_database.cpp)
        target_compile_definitions(debby PUBLIC "DEBBY__LMDB_ENABLED=1")
        target_link_libraries(debby PRIVATE lmdb-static)
    else()
        message(WARNING "No LMDB target found, LMDB support disabled")
    endif()
endif(DEBBY__ENABLE_LMDB)

if (DEBBY__ENABLE_PSQL)
    if (TARGET pgcommon)
        target_compile_definitions(debby PUBLIC "DEBBY__PSQL_ENABLED=1")
        target_link_libraries(debby PRIVATE pq-static pgport pgcommon)
        target_sources(debby PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/data_definition.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/keyvalue_database.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/relational_database.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/result.cpp
            ${CMAKE_CURRENT_LIST_DIR}/src/psql/statement.cpp)
    else()
        message(WARNING "No PostgreSQL target found, PostgreSQL support disabled")
    endif()
endif()

target_include_directories(debby
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/pfs)

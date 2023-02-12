################################################################################
# Copyright (c) 2019-2022 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#      2021.11.24 Initial version.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(debby LANGUAGES CXX C)

option(DEBBY__BUILD_SHARED "Enable build shared library" OFF)
option(DEBBY__BUILD_STATIC "Enable build static library" ON)
option(DEBBY__ENABLE_SQLITE3 "Enable `Sqlite3` backend" ON)
option(DEBBY__ENABLE_ROCKSDB "Enable `RocksDb` backend" OFF)
option(DEBBY__ENABLE_LIBMDBX "Enable `libmdbx` backend" OFF)
option(DEBBY__ENABLE_MAP  "Enable `in-memory` map backend" ON)
option(DEBBY__ENABLE_UNORDERED_MAP  "Enable `in-memory` unordered map backend" ON)

if (NOT PORTABLE_TARGET__CURRENT_PROJECT_DIR)
    set(PORTABLE_TARGET__CURRENT_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if (DEBBY__BUILD_SHARED)
    portable_target(ADD_SHARED ${PROJECT_NAME} ALIAS pfs::debby EXPORTS DEBBY__EXPORTS)
endif()

if (DEBBY__BUILD_STATIC)
    set(STATIC_PROJECT_NAME ${PROJECT_NAME}-static)
    portable_target(ADD_STATIC ${STATIC_PROJECT_NAME} ALIAS pfs::debby::static EXPORTS DEBBY__STATIC)
endif()

if (DEBBY__ENABLE_MAP)
    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/in_memory/map.cpp)
    list(APPEND _debby__definitions "DEBBY__MAP_ENABLED=1")
endif()

if (DEBBY__ENABLE_UNORDERED_MAP)
    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/in_memory/unordered_map.cpp)
    list(APPEND _debby__definitions "DEBBY__UNORDERED_MAP_ENABLED=1")
endif()

if (DEBBY__ENABLE_SQLITE3)
    list(APPEND _debby__sources
        ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/sqlite3.c
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/result.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/statement.cpp)

    list(APPEND _debby__definitions "DEBBY__SQLITE3_ENABLED=1")
endif()

if (NOT TARGET pfs::common)
    portable_target(INCLUDE_PROJECT
        ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/3rdparty/pfs/common/library.cmake)
endif()

if (DEBBY__ENABLE_ROCKSDB)
    if (NOT DEBBY__ROCKSDB_ROOT)
        set(DEBBY__ROCKSDB_ROOT
            "${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/3rdparty/rocksdb"
            CACHE INTERNAL "")
    endif()

    _portable_target_status(${PROJECT_NAME} "RocksDB root: [${DEBBY__ROCKSDB_ROOT}]")

    portable_target(INCLUDE_PROJECT
        ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/cmake/RocksDB.cmake)

    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/rocksdb/database.cpp)
    list(APPEND _debby__definitions "DEBBY__ROCKSDB_ENABLED=1")

    if (DEBBY__BUILD_SHARED)
        portable_target(LINK ${PROJECT_NAME} PRIVATE rocksdb)
    endif()

    if (DEBBY__BUILD_STATIC)
        portable_target(LINK ${STATIC_PROJECT_NAME} PRIVATE rocksdb)
    endif()
endif(DEBBY__ENABLE_ROCKSDB)

if (DEBBY__ENABLE_LIBMDBX)
    if (NOT DEBBY__LIBMDBX_ROOT)
        set(DEBBY__LIBMDBX_ROOT
            "${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/3rdparty/libmdbx"
            CACHE INTERNAL "")
    endif()

    _portable_target_status(${PROJECT_NAME} "libmdbx root: [${DEBBY__LIBMDBX_ROOT}]")

    portable_target(INCLUDE_PROJECT
        ${PORTABLE_TARGET__CURRENT_PROJECT_DIR}/cmake/libmdbx.cmake)

    list(APPEND _debby__sources ${CMAKE_CURRENT_LIST_DIR}/src/libmdbx/database.cpp)
    list(APPEND _debby__definitions "DEBBY__LIBMDBX_ENABLED=1")

    if (DEBBY__BUILD_SHARED)
        portable_target(LINK ${PROJECT_NAME} PRIVATE mdbx-static)
    endif()

    if (DEBBY__BUILD_STATIC)
        portable_target(LINK ${STATIC_PROJECT_NAME} PRIVATE mdbx-static)
    endif()
endif(DEBBY__ENABLE_LIBMDBX)

if (DEBBY__BUILD_SHARED)
    portable_target(SOURCES ${PROJECT_NAME} ${_debby__sources})
    portable_target(DEFINITIONS ${PROJECT_NAME} PUBLIC ${_debby__definitions})
    portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
    portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::common)
endif()

if (DEBBY__BUILD_STATIC)
    portable_target(SOURCES ${STATIC_PROJECT_NAME} ${_debby__sources})
    portable_target(DEFINITIONS ${STATIC_PROJECT_NAME} PUBLIC ${_debby__definitions})
    portable_target(INCLUDE_DIRS ${STATIC_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
    portable_target(LINK ${STATIC_PROJECT_NAME} PUBLIC pfs::common)
endif()

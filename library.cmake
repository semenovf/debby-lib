################################################################################
# Copyright (c) 2019-2022 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#      2021.11.24 Initial version.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(debby CXX C)

option(DEBBY__ENABLE_EXCEPTIONS "Enable exceptions for library" ON)
option(DEBBY__ENABLE_SQLITE3 "Enable `Sqlite3` library" ON)
option(DEBBY__ENABLE_ROCKSDB "Enable `RocksDb` library" OFF)

if (NOT PORTABLE_TARGET__CURRENT_PROJECT_DIR)
    set(PORTABLE_TARGET__CURRENT_PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if (DEBBY__ENABLE_EXCEPTIONS)
    set(PFS__ENABLE_EXCEPTIONS ON CACHE INTERNAL "")
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
endif()

portable_target(LIBRARY ${PROJECT_NAME} ALIAS pfs::debby)
portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::common)
portable_target(EXPORTS ${PROJECT_NAME} DEBBY__EXPORTS DEBBY__STATIC)

if (DEBBY__ENABLE_SQLITE3)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/error.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/sqlite3.c
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/result.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/statement.cpp)

    portable_target(DEFINITIONS ${PROJECT_NAME} PUBLIC "-DDEBBY__SQLITE3_ENABLED=1")
    message(STATUS "`sqlite3` enabled")
endif()

if (DEBBY__ENABLE_ROCKSDB)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/rocksdb/database.cpp)
    portable_target(DEFINITIONS ${PROJECT_NAME} PUBLIC "-DDEBBY__ROCKSDB_ENABLED=1")
    portable_target(LINK ${PROJECT_NAME} PRIVATE rocksdb)

    message(STATUS "`RocksDB` enabled")
endif()

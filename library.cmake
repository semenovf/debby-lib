################################################################################
# Copyright (c) 2019-2021 Vladislav Trifochkin
#
# This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(debby-lib CXX C)

option(PFS_DEBBY__ENABLE_SQLITE3 "Enable `Sqlite3` library" ON)
option(PFS_DEBBY__ENABLE_ROCKSDB "Enable `RocksDb` library" OFF)

portable_target(LIBRARY ${PROJECT_NAME} ALIAS pfs::debby)
portable_target(INCLUDE_DIRS ${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
portable_target(LINK ${PROJECT_NAME} PUBLIC pfs::common)
portable_target(EXPORTS ${PROJECT_NAME} PFS_DEBBY__EXPORTS PFS_DEBBY__STATIC)

if (PFS_DEBBY__ENABLE_SQLITE3)
    portable_target(SOURCES ${PROJECT_NAME}
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/sqlite3.c
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/result.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/statement.cpp)

    portable_target(DEFINITIONS ${PROJECT_NAME} PUBLIC "-DPFS_DEBBY__SQLITE3_ENABLED=1")
    message(STATUS "`sqlite3` enabled")
endif()

# Set default `rocksdb` root path
if (PFS_DEBBY__ENABLE_ROCKSDB AND NOT PFS_ROCKSDB__ROOT)
    set(PFS_ROCKSDB__ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rocksdb")
endif()

if (PFS_DEBBY__ENABLE_ROCKSDB)
    include(FindRocksDB)

    if (PFS_ROCKSDB__STATIC_LIBRARY)
        portable_target(SOURCES ${PROJECT_NAME}
            ${CMAKE_CURRENT_LIST_DIR}/src/rocksdb/database.cpp)
        portable_target(DEFINITIONS ${PROJECT_NAME} PUBLIC "-DPFS_DEBBY__ROCKSDB_ENABLED=1")
        portable_target(LINK ${PROJECT_NAME} PRIVATE ${PFS_ROCKSDB__STATIC_LIBRARY})

        if (PFS_ROCKSDB__INCLUDE_DIR)
            portable_target(INCLUDE_DIRS ${PROJECT_NAME} PRIVATE ${PFS_ROCKSDB__INCLUDE_DIR})
        endif()

        message(STATUS "`RocksDB` enabled")
    endif()
endif()

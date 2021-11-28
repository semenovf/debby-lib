################################################################################
# Copyright (c) 2019-2021 Vladislav Trifochkin
#
# This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(debby-lib CXX C)

option(PFS_DEBBY__ENABLE_SQLITE3 "Enable Sqlite3 library" ON)

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
endif()

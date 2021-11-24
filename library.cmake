################################################################################
# Copyright (c) 2019-2021 Vladislav Trifochkin
#
# This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
################################################################################
cmake_minimum_required (VERSION 3.5)
project(debby-lib CXX C)

option(PFS_DEBBY__ENABLE_SQLITE3 "Enable Sqlite3 library" ON)

if (PFS_DEBBY__ENABLE_SQLITE3)
    list(APPEND SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/sqlite3.c
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/database.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/sqlite3/statement.cpp)
endif()

add_library(${PROJECT_NAME} ${SOURCES})
add_library(pfs::debby ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${PROJECT_NAME} pfs::common)

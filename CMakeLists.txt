################################################################################
# Copyright (c) 2019-2024 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#       2019.11.24 Initial version.
#       2024.10.27 Removed `portable_target` dependency.
#       2024.10.29 Up to C++14 standard.
#       2024.11.12 Min CMake version is 3.15.
#       2024.11.13 Min CMake version is 3.19 (CMakePresets).
################################################################################
cmake_minimum_required (VERSION 3.19)
project(debby-ALL CXX C)

option(DEBBY__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(DEBBY__BUILD_TESTS "Build tests" OFF)
option(DEBBY__ENABLE_COVERAGE "Build tests with coverage support" OFF)
option(DEBBY__DISABLE_FETCH_CONTENT "Disable fetch content if sources of dependencies already exists in the working tree (checks .git subdirectory)" ON)

option(DEBBY__BUILD_SHARED "Enable build shared library" OFF)
option(DEBBY__ENABLE_SQLITE3 "Enable `Sqlite3` backend" ON)
option(DEBBY__ENABLE_SQLITE_FTS5 "Enable support for `FTS5` in `Sqlite3` backend" OFF)
option(DEBBY__ENABLE_ROCKSDB_CXX11 "Enable `RocksDb` backend" OFF)
option(DEBBY__ENABLE_MDBX "Enable `libmdbx` backend" OFF)
option(DEBBY__ENABLE_LMDB "Enable `LMDB` backend" OFF)
option(DEBBY__ENABLE_PSQL "Enable `PostgreSQL` front-end backend" OFF)
option(DEBBY__ENABLE_MAP "Enable `in-memory` map backend" ON)
option(DEBBY__ENABLE_UNORDERED_MAP  "Enable `in-memory` unordered map backend" ON)

if (DEBBY__BUILD_STRICT)
    if (NOT CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 14)
    endif()

    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

add_subdirectory(3rdparty)
add_subdirectory(2ndparty)
include(${CMAKE_CURRENT_LIST_DIR}/library.cmake)

if (DEBBY__BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

include(GNUInstallDirs)

install(TARGETS debby
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

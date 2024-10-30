################################################################################
# Copyright (c) 2019-2024 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#       2019.11.24 Initial version.
#       2024.10.27 Removed `portable_target` dependency.
#       2024.10.29 Up to C++14 standard.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(debby-ALL CXX C)

option(DEBBY__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(DEBBY__BUILD_TESTS "Build tests" OFF)
option(DEBBY__BUILD_DEMO "Build demo" OFF)
option(DEBBY__ENABLE_COVERAGE "Build tests with coverage support" OFF)

if (DEBBY__BUILD_STRICT)
    if (NOT CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 14)
    endif()

    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

if (DEBBY__BUILD_TESTS)
    set(DEBBY__BUILD_SHARED ON)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

include(${CMAKE_CURRENT_LIST_DIR}/library.cmake)

if (DEBBY__BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

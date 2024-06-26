################################################################################
# Copyright (c) 2019-2021 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(debby-ALL CXX C)

option(DEBBY__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(DEBBY__BUILD_TESTS "Build tests" OFF)
option(DEBBY__BUILD_DEMO "Build demo" OFF)
option(DEBBY__ENABLE_COVERAGE "Build tests with coverage support" OFF)

if (NOT PORTABLE_TARGET__ENABLED)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/2ndparty/portable-target/cmake/v2)
    include(PortableTarget)
    portable_target(SET ENABLE_TRACE ON)
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

if (DEBBY__BUILD_STRICT)
    if (NOT CMAKE_CXX_STANDARD)
        set(CMAKE_CXX_STANDARD 11)
    endif()

    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

if (DEBBY__BUILD_TESTS)
    option(DEBBY__BUILD_SHARED "Enable build shared library" ON)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/library.cmake)

if (DEBBY__BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#       2021.11.24 Initial version.
#       2021.12.10 Refactored for using portable_target `ADD_TEST`.
#       2024.10.27 Removed `portable_target` dependency.
#       2024.11.20 Removed obsolete tests.
################################################################################
project(debby-TESTS CXX C)

list(APPEND TESTS
    keyvalue_database
    relational_database
    statement
    data_definition)

foreach (target ${TESTS})
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE pfs::debby)
    add_test(NAME ${target} COMMAND ${target})

    if (PFS__ENABLE_COVERAGE)
        target_compile_options(${target} PRIVATE -g -O0 --coverage)
        target_link_libraries(${target} PRIVATE -g -O0 --coverage)
    endif()
endforeach()

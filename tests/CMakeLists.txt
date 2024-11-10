################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#       2021.11.24 Initial version.
#       2021.12.10 Refactored for using portable_target `ADD_TEST`.
#       2024.10.27 Removed `portable_target` dependency.
################################################################################
project(debby-TESTS CXX C)

list(APPEND TESTS
    keyvalue_database
    # psql_affinity
    # psql_cast_traits
    relational_database
    # sqlite3_affinity
    # sqlite3_cast_traits
    statement
    # cached_statement
    # unified_value
)

foreach (target ${TESTS})
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE pfs::debby)
    add_test(NAME ${target} COMMAND ${target})

    if (PFS__ENABLE_COVERAGE)
        target_compile_options(${target} PRIVATE -g -O0 --coverage)
        target_link_libraries(${target} PRIVATE -g -O0 --coverage)
    endif()
endforeach()

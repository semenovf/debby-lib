################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#      2021.11.24 Initial version.
#      2021.12.10 Refactored for using portable_target `ADD_TEST`.
################################################################################
project(debby-lib-TESTS CXX C)

list(APPEND TESTS
    keyvalue_database
    relational_database
    sqlite3_affinity
    sqlite3_cast_traits
    statement
    cached_statement
    unified_value)

foreach (t ${TESTS})
    portable_target(ADD_TEST ${t}
        ENABLE_COVERAGE ${DEBBY__ENABLE_COVERAGE}
        SOURCES ${t}.cpp ${${t}_SOURCES}
        LINK pfs::debby)
endforeach()

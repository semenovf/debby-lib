################################################################################
# Copyright (c) 2021,2022 Vladislav Trifochkin
#
# This file is part of `debby-lib`.
#
# Changelog:
#      2021.11.24 Initial version.
#      2021.12.10 Refactored for using portable_target `ADD_TEST`.
################################################################################
project(debby-TESTS CXX C)

list(APPEND TESTS
    keyvalue_database
    psql_affinity
    psql_cast_traits
    relational_database
    sqlite3_affinity
    sqlite3_cast_traits
    statement
    cached_statement
    unified_value)

if (DEBBY__ENABLE_PSQL)
    list(APPEND TESTS psql)
endif()

if (TARGET pfs::debby)
    set(_debby_lib pfs::debby)
elseif(TARGET pfs::debby::static)
    set(_debby_lib pfs::debby::static)
endif()

foreach (t ${TESTS})
    portable_target(ADD_TEST ${t}
        ENABLE_COVERAGE ${DEBBY__ENABLE_COVERAGE}
        SOURCES ${t}.cpp ${${t}_SOURCES}
        LINK ${_debby_lib})
endforeach()

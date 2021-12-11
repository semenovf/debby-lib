################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
#
# Changelog:
#      2021.11.24 Initial version.
#      2021.12.10 Refactored for using portable_target `ADD_TEST`.
################################################################################
project(debby-lib-TESTS CXX C)

list(APPEND TESTS
    cached_statement
    database
    sqlite3_affinity
    sqlite3_cast_traits
    statement)

if (PFS_DEBBY__ENABLE_ROCKSDB)
    list(APPEND TEST_NAMES rocksdb_database)
endif()

foreach (t ${TESTS})
    portable_target(ADD_TEST ${t}
        ENABLE_COVERAGE ${PFS_DEBBY__ENABLE_COVERAGE}
        SOURCES ${t}.cpp ${${t}_SOURCES}
        LINK pfs::debby)
endforeach()

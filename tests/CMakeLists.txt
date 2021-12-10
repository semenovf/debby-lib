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

list(APPEND TEST_NAMES
    database
    cached_statement
    sqlite3_affinity
    statement)

if (PFS_DEBBY__ENABLE_ROCKSDB)
    list(APPEND TEST_NAMES rocksdb_database)
endif()

foreach (name ${TEST_NAMES})
    portable_target(ADD_TEST ${name}
        PFS_DEBBY__ENABLE_COVERAGE ${PFS_DEBBY__ENABLE_COVERAGE}
        SOURCES ${name}.cpp ${${name}_SOURCES}
        LINK pfs::debby)
endforeach()

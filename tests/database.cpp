////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"

#if DEBBY__SQLITE3_ENABLED
#   include "pfs/debby/sqlite3/database.hpp"
#endif

#if DEBBY__ROCKSDB_ENABLED
#   include "pfs/debby/rocksdb/database.hpp"
#endif

namespace fs = pfs::filesystem;

namespace {
std::string BAD_DB_PATH {"!@#$%^&"};
} // namespace


template <typename T>
void check_constructor (pfs::filesystem::path const & db_path)
{
    using database_t = T;

    bool create_if_missing = false;

    {
        debby::error err;
        REQUIRE_FALSE(database_t{BAD_DB_PATH, create_if_missing, & err});
        REQUIRE_EQ(err.code(), make_error_code(debby::errc::database_not_found));
    }

#if DEBBY__EXCEPTIONS_ENABLED
    {
        REQUIRE_THROWS_AS(database_t{BAD_DB_PATH, create_if_missing}, debby::error);

        try {
            db.open(BAD_DB_PATH, create_if_missing);
        } catch (debby::exception ex) {
            REQUIRE_EQ_ERROR_CODE(ex.code(), debby::errc::database_not_found);
        }
    }
#endif

    if (fs::exists(db_path) && fs::is_regular_file(db_path))
        fs::remove(db_path);

    create_if_missing = true; // is default for database open

    {
#if DEBBY__EXCEPTIONS_ENABLED
        REQUIRE_NOTHROW(database_t{db_path, create_if_missing});
#else
        REQUIRE(database_t{db_path, create_if_missing});
#endif
    }

    {
        database_t db {db_path};

        REQUIRE(db);

        // Move constructor
        database_t db_other{std::move(db)};

        REQUIRE_FALSE(db);
        REQUIRE(db_other);

        // Default constructor
        database_t db_another;

        REQUIRE_FALSE(db_another);

        // Move assignment
        db_another = std::move(db_other);

        REQUIRE_FALSE(db_other);
        REQUIRE(db_another);
    }
}

#if DEBBY__SQLITE3_ENABLED
TEST_CASE("sqlite3 database constructor") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3.db");
    check_constructor<debby::sqlite3::database>(db_path);
}
#endif

#if DEBBY__ROCKSDB_ENABLED
TEST_CASE("rocksdb database constructor") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-rocksdb.db");
    check_constructor<debby::rocksdb::database>(db_path);
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/debby/relational_database.hpp"
#include <algorithm>

#if DEBBY__SQLITE3_ENABLED
#   include "pfs/debby/backend/sqlite3/database.hpp"
#endif

#if DEBBY__PSQL_ENABLED
#   include "pfs/debby/backend/psql/database.hpp"
#   include "psql_support.hpp"
#endif

namespace fs = pfs::filesystem;

namespace {
std::string const CREATE_TABLE_ONE {
    R"(CREATE TABLE IF NOT EXISTS one (col INTEGER))"
};

std::string const CREATE_TABLE_TWO {
    R"(CREATE TABLE IF NOT EXISTS "two" ("col" INTEGER))"
};

std::string const CREATE_TABLE_THREE {
    R"(CREATE TABLE IF NOT EXISTS "three" ("col" INTEGER))"
};
} // namespace

template <typename RelationalDatabaseType>
void check (RelationalDatabaseType & db)
{
    db.remove_all();
    REQUIRE_FALSE(db.exists("one"));
    REQUIRE_FALSE(db.exists("two"));
    REQUIRE_FALSE(db.exists("three"));
    REQUIRE_FALSE(db.exists("four"));

    db.query(CREATE_TABLE_ONE);
    db.query(CREATE_TABLE_TWO);
    db.query(CREATE_TABLE_THREE);

    REQUIRE(db.exists("one"));
    REQUIRE(db.exists("two"));
    REQUIRE(db.exists("three"));
    REQUIRE_FALSE(db.exists("four"));

    {
        auto tables = db.tables();
        CHECK_NE(std::find(tables.begin(), tables.end(), "one"), std::end(tables));
        CHECK_NE(std::find(tables.begin(), tables.end(), "two"), std::end(tables));
        CHECK_NE(std::find(tables.begin(), tables.end(), "three"), std::end(tables));
        CHECK_EQ(std::find(tables.begin(), tables.end(), "four"), std::end(tables));
    }

    {
        auto tables = db.tables("^t.*");
        CHECK_EQ(std::find(tables.begin(), tables.end(), "one"), std::end(tables));
        CHECK_NE(std::find(tables.begin(), tables.end(), "two"), std::end(tables));
        CHECK_NE(std::find(tables.begin(), tables.end(), "three"), std::end(tables));
        CHECK_EQ(std::find(tables.begin(), tables.end(), "ten"), std::end(tables));
    }

    db.remove("two");
    REQUIRE_FALSE(db.exists("two"));

    db.remove_all();
    REQUIRE_FALSE(db.exists("one"));
    REQUIRE_FALSE(db.exists("two"));
    REQUIRE_FALSE(db.exists("three"));
}

#if DEBBY__SQLITE3_ENABLED
TEST_CASE("sqlite3") {
    using database_t = debby::relational_database<debby::backend::sqlite3::database>;

    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3.db");
    database_t::wipe(db_path);

    auto db = database_t::make(db_path);

    REQUIRE(db);

    check(db);

    database_t::wipe(db_path);
}
#endif

#if DEBBY__PSQL_ENABLED
TEST_CASE("PostgreSQL") {
    using database_t = debby::relational_database<debby::backend::psql::database>;

    auto conninfo = psql_conninfo();
    auto db = database_t::make(conninfo.cbegin(), conninfo.cend());

    if (!db) {
        MESSAGE(preconditions_notice());
    }

    REQUIRE(db);

    check(db);
}
#endif

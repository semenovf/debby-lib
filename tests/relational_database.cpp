////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2024 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
//      2024.10.29 V2 started.
//      2024.10.30 Fixed for sqlite3 database.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/debby/relational_database.hpp"
#include <pfs/filesystem.hpp>

#if DEBBY__SQLITE3_ENABLED
#   include "pfs/debby/sqlite3.hpp"
#endif
//
#if DEBBY__PSQL_ENABLED
#   include "pfs/debby/psql.hpp"
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
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3.db");
    debby::sqlite3::wipe(db_path);

    auto db = debby::sqlite3::make(db_path);

    REQUIRE(db);

    check(db);

    debby::sqlite3::wipe(db_path);
}
#endif

#if DEBBY__PSQL_ENABLED
TEST_CASE("PostgreSQL") {
    auto conninfo = psql_conninfo();
    auto db = debby::psql::make(conninfo.cbegin(), conninfo.cend());

    if (!db) {
        MESSAGE(preconditions_notice());
    }

    REQUIRE(db);

    check(db);
}
#endif

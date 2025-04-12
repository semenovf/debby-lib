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
    R"(CREATE TABLE IF NOT EXISTS one (i16 SMALLINT, i32 INTEGER, i64 BIGINT))"
};

std::string const CREATE_TABLE_TWO {
    R"(CREATE TABLE IF NOT EXISTS two (f32 REAL, f64 DOUBLE PRECISION))"
};

std::string const CREATE_TABLE_THREE {
    R"(CREATE TABLE IF NOT EXISTS three (col INTEGER))"
};
} // namespace

template <typename RelationalDatabaseType>
void check (RelationalDatabaseType & db_opened)
{
    // Default constructor
    RelationalDatabaseType db;

    REQUIRE_FALSE(db);

    // Move assignment operator
    db = std::move(db_opened);

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

    db.query("INSERT INTO one (i16, i32, i64) VALUES (42, 100042, 10000000042)");
    db.query("INSERT INTO one (i16, i32, i64) VALUES (43, 100043, 10000000043)");
    db.query("INSERT INTO one (i16, i32, i64) VALUES (44, 100044, 10000000044)");

    {
        auto res = db.exec("SELECT * from one");

        std::int16_t i16 = 42;
        std::int32_t i32 = 100042;
        std::int64_t i64 = 10000000042L;

        while (res.has_more()) {
            auto x = res.template get<std::int16_t>(0);
            auto y = res.template get<std::int32_t>(1);
            auto z = res.template get<std::int64_t>(2);
            CHECK_EQ(*x, i16++);
            CHECK_EQ(*y, i32++);
            CHECK_EQ(*z, i64++);
            res.next();
        }

        // result destroyed here, so database not locked (for SQLite)
    }

    db.query("INSERT INTO two (f32, f64) VALUES (3.14159, 3.14159)");

    {
        auto res = db.exec("SELECT * from two");

        while (res.has_more()) {
            auto x = res.template get<float>(0);
            auto y = res.template get<double>(1);
            CHECK_EQ(*x, float{3.14159});
            CHECK_EQ(*y, double{3.14159});
            res.next();
        }

        // result destroyed here, so database not locked (for SQLite)
    }

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
    using database_t = debby::relational_database<debby::backend_enum::sqlite3>;
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
    debby::error err;
    auto conninfo = psql_conninfo();
    auto db = debby::psql::make(conninfo.cbegin(), conninfo.cend(), & err);

    if (!db) {
        WARN(db);
        MESSAGE(err.what());
        MESSAGE(preconditions_notice());
        return;
    }

    REQUIRE(db);

    check(db);
}
#endif

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

    std::map<std::string, std::string> conninfo = {
          {"host", "localhost"}
        , {"port", "5432"} // Default port
        , {"user", "test"}
        , {"password", "12345678"}
        , {"dbname", "postgres"}
    };

    std::string db_name {"debby"};
    debby::error err;
    database_t::wipe(db_name, conninfo.cbegin(), conninfo.cend(), & err);

    if (err) {
        MESSAGE("\n"
            "=======================================================================================\n"
            "Perhaps it was not possible to connect to the database\n"
            "For testing purposes, the following prerequisites must be met:\n"
            "\t* PostgresSQL instance must be started on localhost on port 5432;\n"
            "\t* `test` login must be available with roles ...;\n"
            "\t* password for test login must be `12345678`.\n"
            "\n"
            "Below instructions can help to create `test` user/login\n"
            "$ psql --host=localhost --port=5432 --user=postgres\n"
            "postgres=# CREATE ROLE test WITH LOGIN NOSUPERUSER CREATEDB NOCREATEROLE NOINHERIT NOREPLICATION CONNECTION LIMIT -1 PASSWORD '12345678'\n"
            "postgres=# \\q\n\n"
            "Check connection:\n"
            "$ psql --host=localhost --port=5432 --user=test --database=postgres\n"
            "=======================================================================================\n");
    }

    auto db = database_t::make(conninfo.cbegin(), conninfo.cend());

    REQUIRE(db);

    check(db);
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.26 Initial version.
////////////////////////////////////////////////////////////////////////////////
#if DEBBY__PSQL_ENABLED

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/debby/relational_database.hpp"
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/backend/psql/database.hpp"
// #include "pfs/string_view.hpp"
// #include "pfs/debby/backend/psql/affinity_traits.hpp"
// #include "pfs/debby/backend/psql/sha256_traits.hpp"
// #include "pfs/debby/backend/psql/time_point_traits.hpp"

// using namespace debby::backend::psql;

namespace {
std::string const CREATE_TABLE_ONE {
    "CREATE TABLE IF NOT EXISTS one (col INTEGER)"
};

std::string const CREATE_TABLE_TWO {
    "CREATE TABLE IF NOT EXISTS two (col INTEGER)"
};

std::string const CREATE_TABLE_THREE {
    "CREATE TABLE IF NOT EXISTS three (col INTEGER)"
};
} // namespace

TEST_CASE("PostgreSQL database constructor")
{
    using database_t = debby::relational_database<debby::backend::psql::database>;

    std::map<std::string, std::string> conninfo = {
          {"host", "localhost"}
        , {"port", "5432"}       // Default port
        , {"user", "postgres"} // , {"user", "test"}
        , {"password", "rdflhfnehf"} // , {"password", "12345678"}
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

    database_t::wipe(db_name, conninfo.cbegin(), conninfo.cend());
}

#endif


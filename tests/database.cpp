////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/debby/sqlite3/database.hpp"
#include <algorithm>

namespace {
std::string const CREATE_TABLE_ONE {
    "CREATE TABLE IF NOT EXISTS `one` (`col` INTEGER)"
};

std::string const CREATE_TABLE_TWO {
    "CREATE TABLE IF NOT EXISTS `two` (`col` INTEGER)"
};

std::string const CREATE_TABLE_THREE {
    "CREATE TABLE IF NOT EXISTS `three` (`col` INTEGER)"
};
} // namespace

namespace fs = pfs::filesystem;
using database_t = pfs::debby::sqlite3::database;

class database_wrapper
{
    database_t * _db {nullptr};

public:
    database_wrapper (database_t * db)
        : _db(db)
    {}

    ~database_wrapper ()
    {
        auto last_error = _db->last_error();

        if (!last_error.empty())
            fmt::print(stderr, "ERROR: {}\n", last_error);
    }

    database_t * operator -> () { return _db; }
};

TEST_CASE("database") {

    auto db_path = fs::temp_directory_path() / "debby.db";

    database_t db;
    database_wrapper dbw {& db};

    REQUIRE(dbw->open(db_path));
    REQUIRE(dbw->is_opened());
    REQUIRE(dbw->clear());
    REQUIRE(dbw->query(CREATE_TABLE_ONE));
    REQUIRE(dbw->query(CREATE_TABLE_TWO));
    REQUIRE(dbw->query(CREATE_TABLE_THREE));

    CHECK(dbw->exists("one"));
    CHECK(dbw->exists("two"));
    CHECK(dbw->exists("three"));
    CHECK_FALSE(dbw->exists("four"));

    {
        auto tables = dbw->tables();
        CHECK(std::find(tables.begin(), tables.end(), "one") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "two") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "three") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "four") == std::end(tables));
    }

    {
        auto tables = dbw->tables("^t.*");
        CHECK(std::find(tables.begin(), tables.end(), "one") == std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "two") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "three") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "ten") == std::end(tables));
    }

    dbw->close();
    REQUIRE_FALSE(dbw->is_opened());
}

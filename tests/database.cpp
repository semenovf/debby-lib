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

TEST_CASE("database") {
    namespace fs = pfs::filesystem;
    using database_t = pfs::debby::sqlite3::database;

    auto db_path = fs::temp_directory_path() / "debby.db";

    database_t db;

    REQUIRE(db.open(db_path));
    REQUIRE(db.is_opened());
    REQUIRE(db.query(CREATE_TABLE_ONE));
    REQUIRE(db.query(CREATE_TABLE_TWO));
    REQUIRE(db.query(CREATE_TABLE_THREE));

    CHECK(db.exists("one"));
    CHECK(db.exists("two"));
    CHECK(db.exists("three"));
    CHECK_FALSE(db.exists("four"));

    {
        auto tables = db.tables();
        CHECK(std::find(tables.begin(), tables.end(), "one") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "two") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "three") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "four") == std::end(tables));
    }

    {
        auto tables = db.tables("^t.*");
        CHECK(std::find(tables.begin(), tables.end(), "one") == std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "two") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "three") != std::end(tables));
        CHECK(std::find(tables.begin(), tables.end(), "ten") == std::end(tables));
    }

    REQUIRE(db.clear());

    db.close();
    REQUIRE_FALSE(db.is_opened());
}


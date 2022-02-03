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
#include <algorithm>

#if DEBBY__SQLITE3_ENABLED
#   include "pfs/debby/sqlite3/database.hpp"
#endif

namespace fs = pfs::filesystem;

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

template <typename T>
void check (pfs::filesystem::path const & db_path)
{
    using database_t = T;

    if (fs::exists(db_path) && fs::is_regular_file(db_path))
        fs::remove(db_path);

    database_t db {db_path};

    REQUIRE(db);

    REQUIRE(db.remove_all());
    REQUIRE_FALSE(db.exists("one"));
    REQUIRE_FALSE(db.exists("two"));
    REQUIRE_FALSE(db.exists("three"));
    REQUIRE_FALSE(db.exists("four"));

    REQUIRE(db.query(CREATE_TABLE_ONE));
    REQUIRE(db.query(CREATE_TABLE_TWO));
    REQUIRE(db.query(CREATE_TABLE_THREE));

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

    REQUIRE(db.remove("two"));
    REQUIRE_FALSE(db.exists("two"));

    REQUIRE(db.remove_all());
    REQUIRE_FALSE(db.exists("one"));
    REQUIRE_FALSE(db.exists("two"));
    REQUIRE_FALSE(db.exists("three"));
}

#if DEBBY__SQLITE3_ENABLED
TEST_CASE("sqlite3 database constructor") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3.db");
    check<debby::sqlite3::database>(db_path);
}
#endif

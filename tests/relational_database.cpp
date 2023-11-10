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
#include <algorithm>

#include "pfs/debby/relational_database.hpp"

#if DEBBY__SQLITE3_ENABLED
#   include "pfs/debby/backend/sqlite3/database.hpp"
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

template <typename Backend>
void check (pfs::filesystem::path const & db_path)
{
    using database_t = debby::relational_database<Backend>;

    database_t::wipe(db_path);

    auto db = database_t::make(db_path);

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

    database_t::wipe(db_path);
}

#if DEBBY__SQLITE3_ENABLED
TEST_CASE("sqlite3 database constructor") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3.db");
    check<debby::backend::sqlite3::database>(db_path);
}
#endif

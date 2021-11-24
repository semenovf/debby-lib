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

TEST_CASE("database") {
    namespace fs = pfs::filesystem;
    using database_t = pfs::debby::sqlite3::database;

    auto db_path = fs::temp_directory_path() / "debby.db";

    database_t db;

    REQUIRE(db.open(db_path));
    REQUIRE(db.is_opened());

    db.close();
    REQUIRE_FALSE(db.is_opened());
}


////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.07 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/debby/rocksdb/database.hpp"
#include <limits>

TEST_CASE("rocksdb_database") {
    namespace fs = pfs::filesystem;
    using database_t = pfs::debby::rocksdb::database;

    auto db_path = fs::temp_directory_path() / "debby-rocksdb.db";

    database_t db;

    REQUIRE(db.open(db_path));
    REQUIRE(db.is_opened());
    // REQUIRE(db.clear());

    REQUIRE(db.set("bool"  , true));
    REQUIRE(db.set("int8"  , std::numeric_limits<std::int8_t>::min()));
    REQUIRE(db.set("uint8" , std::numeric_limits<std::uint8_t>::max()));
    REQUIRE(db.set("int16" , std::numeric_limits<std::int16_t>::min()));
    REQUIRE(db.set("uint16", std::numeric_limits<std::uint16_t>::max()));
    REQUIRE(db.set("int32" , std::numeric_limits<std::int32_t>::min()));
    REQUIRE(db.set("uint32", std::numeric_limits<std::uint32_t>::max()));
    REQUIRE(db.set("int64" , std::numeric_limits<std::int64_t>::min()));
    REQUIRE(db.set("uint64", std::numeric_limits<std::uint64_t>::max()));
    REQUIRE(db.set("float" , static_cast<float>(3.14159)));
    REQUIRE(db.set("double", static_cast<double>(3.14159)));
    REQUIRE(db.set("text"  , std::string{"Hello"}));
    REQUIRE(db.set("cstr"  , "World"));

    db.close();
    REQUIRE_FALSE(db.is_opened());
}



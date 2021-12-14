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

    database_t db;

    REQUIRE_THROWS_AS(db.open("!@#$%^&/!@#$%^&"), pfs::runtime_error);
    REQUIRE_NOTHROW(db.close());

    auto db_path = fs::temp_directory_path() / "debby-rocksdb.db";

    if (fs::exists(db_path) && fs::is_regular_file(db_path))
        fs::remove(db_path);

    REQUIRE_NOTHROW(db.open(db_path));
    REQUIRE(db.is_opened());

    REQUIRE_NOTHROW(db.set("bool"  , true));
    REQUIRE_NOTHROW(db.set("int8"  , std::numeric_limits<std::int8_t>::min()));
    REQUIRE_NOTHROW(db.set("uint8" , std::numeric_limits<std::uint8_t>::max()));
    REQUIRE_NOTHROW(db.set("int16" , std::numeric_limits<std::int16_t>::min()));
    REQUIRE_NOTHROW(db.set("uint16", std::numeric_limits<std::uint16_t>::max()));
    REQUIRE_NOTHROW(db.set("int32" , std::numeric_limits<std::int32_t>::min()));
    REQUIRE_NOTHROW(db.set("uint32", std::numeric_limits<std::uint32_t>::max()));
    REQUIRE_NOTHROW(db.set("int64" , std::numeric_limits<std::int64_t>::min()));
    REQUIRE_NOTHROW(db.set("uint64", std::numeric_limits<std::uint64_t>::max()));
    REQUIRE_NOTHROW(db.set("float" , static_cast<float>(3.14159)));
    REQUIRE_NOTHROW(db.set("double", static_cast<double>(3.14159)));
    REQUIRE_NOTHROW(db.set("text"  , std::string{"Hello"}));
    REQUIRE_NOTHROW(db.set("empty" , std::string{""}));
    REQUIRE_NOTHROW(db.set("cstr"  , "World"));

    REQUIRE_NOTHROW(db.get<int>("unknown"));
    CHECK(db.get<int>("unknown").has_value() == false);

    CHECK(*db.get<bool>("bool") == true);
    CHECK(*db.get<std::int8_t>("int8") == std::numeric_limits<std::int8_t>::min());
    CHECK(*db.get<std::uint8_t>("uint8") == std::numeric_limits<std::uint8_t>::max());
    CHECK(*db.get<std::int16_t>("int16") == std::numeric_limits<std::int16_t>::min());
    CHECK(*db.get<std::uint16_t>("uint16") == std::numeric_limits<std::uint16_t>::max());
    CHECK(*db.get<std::int32_t>("int32") == std::numeric_limits<std::int32_t>::min());
    CHECK(*db.get<std::uint32_t>("uint32") == std::numeric_limits<std::uint32_t>::max());
    CHECK(*db.get<std::int64_t>("int64") == std::numeric_limits<std::int64_t>::min());
    CHECK(*db.get<std::uint64_t>("uint64") == std::numeric_limits<std::uint64_t>::max());
    CHECK(std::abs(*db.get<float>("float") - static_cast<float>(3.14159)) < float{0.001});
    CHECK(std::abs(*db.get<double>("double") - static_cast<double>(3.14159)) < double(0.001));

    CHECK(*db.get<std::string>("text") == std::string{"Hello"});
    CHECK(db.get<std::string>("empty")->empty());

    CHECK(db.remove("text"));
    CHECK(db.get<std::string>("text").has_value() == false);

    REQUIRE_NOTHROW(db.close());
    REQUIRE_FALSE(db.is_opened());
}

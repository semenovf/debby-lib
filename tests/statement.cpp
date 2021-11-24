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
#include "pfs/debby/sqlite3/statement.hpp"
#include <limits>

namespace {
std::string TABLE_NAME {"test"};

std::string const CREATE_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`null` INTEGER"
        ", `bool` INTEGER"
        ", `int8` INTEGER"
        ", `uint8` INTEGER"
        ", `int16` INTEGER"
        ", `uint16` INTEGER"
        ", `int32` INTEGER"
        ", `uint32` INTEGER"
        ", `int64` INTEGER"
        ", `uint64` INTEGER"
        ", `text` TEXT"
        ", `cstr` TEXT)"
};

std::string const INSERT {
    "INSERT INTO `{}` (`null`, `bool`, `int8`, `uint8`"
        ", `int16`, `uint16`, `int32`, `uint32`"
        ", `int64`, `uint64`, `text`, `cstr`)"
    " VALUES (:null, :bool, :int8, :uint8"
        ", :int16, :uint16, :int32, :uint32"
        ", :int64, :uint64, :text, :cstr)"
};

} // namespace

TEST_CASE("statement") {
    namespace fs = pfs::filesystem;
    using database_t = pfs::debby::sqlite3::database;

    auto db_path = fs::temp_directory_path() / "debby.db";

    database_t db;

    REQUIRE(db.open(db_path));

    {
        auto stmt = db.prepare(fmt::format(CREATE_TABLE, TABLE_NAME));

        REQUIRE(stmt);
        REQUIRE(stmt.exec());
    }

    {
        auto stmt = db.prepare(fmt::format(INSERT, TABLE_NAME));

        if (!stmt) {
            fmt::print(stderr, "ERROR: {}\n", db.last_error());
        }

        REQUIRE(stmt);

        bool success = stmt.bind(":null", nullptr)
            && stmt.bind(":bool", true)
            && stmt.bind(":int8", std::numeric_limits<std::int8_t>::min())
            && stmt.bind(":uint8", std::numeric_limits<std::uint8_t>::max())
            && stmt.bind(":int16", std::numeric_limits<std::int16_t>::min())
            && stmt.bind(":uint16", std::numeric_limits<std::uint16_t>::max())
            && stmt.bind(":int32", std::numeric_limits<std::int32_t>::min())
            && stmt.bind(":uint32", std::numeric_limits<std::uint32_t>::max())
            && stmt.bind(":int64", std::numeric_limits<std::int64_t>::min())
            && stmt.bind(":uint64", std::numeric_limits<std::uint64_t>::max())
            && stmt.bind(":text", std::string{"Hello"})
            && stmt.bind(":cstr", "World");

        if (!success) {
            fmt::print(stderr, "ERROR: {}\n", stmt.last_error());
        }

        REQUIRE(success);
        REQUIRE(stmt.exec());
    }

    db.close();
}


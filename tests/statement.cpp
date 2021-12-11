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
#include "pfs/debby/sqlite3/input_record.hpp"
#include "pfs/debby/sqlite3/database.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include <cmath>
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
        ", `float` REAL"
        ", `double` REAL"
        ", `text` TEXT"
        ", `cstr` TEXT)"
};

std::string const INSERT {
    "INSERT INTO `{}` (`null`, `bool`, `int8`, `uint8`"
        ", `int16`, `uint16`, `int32`, `uint32`"
        ", `int64`, `uint64`, `float`, `double`"
        ", `text`, `cstr`)"
    " VALUES (:null, :bool, :int8, :uint8"
        ", :int16, :uint16, :int32, :uint32"
        ", :int64, :uint64, :float, :double"
        ", :text, :cstr)"
};

std::string const SELECT_ALL {
    "SELECT * FROM `{}`"
};

std::string const SELECT {
    "SELECT * FROM `{}` WHERE `int8` = :int8"
};

} // namespace

TEST_CASE("statement") {
    namespace fs = pfs::filesystem;
    using database_t = pfs::debby::sqlite3::database;

    auto db_path = fs::temp_directory_path() / "debby.db";

    database_t db;

    REQUIRE(db.open(db_path));
    db.clear();

    {
        auto stmt = db.prepare(fmt::format(CREATE_TABLE, TABLE_NAME));

        REQUIRE(stmt);
        auto result = stmt.exec();
        REQUIRE(result.is_done());
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
            && stmt.bind(":float", static_cast<float>(3.14159))
            && stmt.bind(":double", static_cast<double>(3.14159))
            && stmt.bind(":text", std::string{"Hello"})
            && stmt.bind(":cstr", "World");

        if (!success) {
            fmt::print(stderr, "ERROR: {}\n", stmt.last_error());
        }

        REQUIRE(success);
        REQUIRE(stmt);
        auto result = stmt.exec();
        REQUIRE(result.is_done());
    }

    {
        auto stmt = db.prepare(fmt::format(SELECT_ALL, TABLE_NAME));

        if (!stmt) {
            fmt::print(stderr, "ERROR: {}\n", db.last_error());
        }

        REQUIRE(stmt);

        auto result = stmt.exec();
        REQUIRE(result.has_more());
        REQUIRE_FALSE(result.is_done());
        REQUIRE_FALSE(result.is_error());

        CHECK_EQ(result.column_count(), 14);

        CHECK_EQ(result.column_name(-1), std::string{});
        CHECK_EQ(result.column_name(14), std::string{});

        CHECK_EQ(result.column_name(0), std::string{"null"});
        CHECK_EQ(result.column_name(1), std::string{"bool"});
        CHECK_EQ(result.column_name(2), std::string{"int8"});
        CHECK_EQ(result.column_name(3), std::string{"uint8"});
        CHECK_EQ(result.column_name(4), std::string{"int16"});
        CHECK_EQ(result.column_name(5), std::string{"uint16"});
        CHECK_EQ(result.column_name(6), std::string{"int32"});
        CHECK_EQ(result.column_name(7), std::string{"uint32"});
        CHECK_EQ(result.column_name(8), std::string{"int64"});
        CHECK_EQ(result.column_name(9), std::string{"uint64"});
        CHECK_EQ(result.column_name(10), std::string{"float"});
        CHECK_EQ(result.column_name(11), std::string{"double"});
        CHECK_EQ(result.column_name(12), std::string{"text"});
        CHECK_EQ(result.column_name(13), std::string{"cstr"});

        while (result.has_more()) {
            // Error for nonexistent column
            REQUIRE_FALSE(result.get<int>("unknown"));
            CHECK(result.get<int>("unknown").error() == true);

            // Error or column contains `null`
            REQUIRE_FALSE(result.get<int>("null"));

            // Column `null` is INTEGER but contains null value, so error() returns true
            CHECK(result.get<int>("null").error() == false);

            // It is no matter the column type if it contains `null`
            CHECK(result.get<float>("null").error() == false);

            CHECK(*result.get<bool>("bool") == true);
            CHECK(*result.get<std::int8_t>("int8") == std::numeric_limits<std::int8_t>::min());
            CHECK(*result.get<std::uint8_t>("uint8") == std::numeric_limits<std::uint8_t>::max());
            CHECK(*result.get<std::int16_t>("int16") == std::numeric_limits<std::int16_t>::min());
            CHECK(*result.get<std::uint16_t>("uint16") == std::numeric_limits<std::uint16_t>::max());
            CHECK(*result.get<std::int32_t>("int32") == std::numeric_limits<std::int32_t>::min());
            CHECK(*result.get<std::uint32_t>("uint32") == std::numeric_limits<std::uint32_t>::max());
            CHECK(*result.get<std::int64_t>("int64") == std::numeric_limits<std::int64_t>::min());
            CHECK(*result.get<std::uint64_t>("uint64") == std::numeric_limits<std::uint64_t>::max());
            CHECK(std::abs(*result.get<float>("float") - static_cast<float>(3.14159)) < float{0.001});
            CHECK(std::abs(*result.get<double>("double") - static_cast<double>(3.14159)) < double(0.001));
            CHECK(*result.get<std::string>("text") == std::string{"Hello"});

            // And using `input_record`
            pfs::debby::sqlite3::input_record in {result};

            {
                bool b;
                CHECK(in.assign("bool").to(b));
                CHECK(b == true);
            }

            {
                bool b;
                in["bool"] >> b;
                CHECK(b == true);
            }

            {
                std::int8_t i8;
                CHECK(in.assign("int8").to(i8));
                CHECK(i8 == std::numeric_limits<std::int8_t>::min());
            }

            {
                std::int8_t i8;
                in["int8"] >> i8;
                CHECK(i8 == std::numeric_limits<std::int8_t>::min());
            }

            {
                std::uint64_t u64;
                CHECK(in.assign("uint64").to(u64));
                CHECK(u64 == std::numeric_limits<std::uint64_t>::max());
            }

            {
                std::uint64_t u64;
                in["uint64"] >> u64;
                CHECK(u64 == std::numeric_limits<std::uint64_t>::max());
            }

            {
                float f;
                CHECK(in.assign("float").to(f));
                CHECK(std::abs(f - static_cast<float>(3.14159)) < float{0.001});
            }

            {
                double f;
                in["double"] >> f;
                CHECK(std::abs(f - static_cast<double>(3.14159)) < double{0.001});
            }

            {
                std::string s;
                in["text"] >> s;
                CHECK(s == "Hello");
            }

            result.next();
        }

        CHECK(result.is_done());
    }

    {
        for (int i = 0; i < 2; i++) {
            auto stmt = db.prepare(fmt::format(SELECT, TABLE_NAME), true);

            if (!stmt) {
                fmt::print(stderr, "ERROR: {}\n", db.last_error());
            }

            REQUIRE(stmt);

            bool success = stmt.bind(":int8", std::numeric_limits<std::int8_t>::min());

            if (!success) {
                fmt::print(stderr, "ERROR: {}\n", stmt.last_error());
            }

            REQUIRE(success);
            REQUIRE(stmt);
            auto result = stmt.exec();

            while (result.has_more())
                result.next();

            REQUIRE(result.is_done());
        }
    }

    db.close();
}


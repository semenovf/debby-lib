////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.11.24 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/debby/relational_database.hpp"
#include "pfs/debby/statement.hpp"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include <cmath>
#include <limits>

#if DEBBY__SQLITE3_ENABLED
#   include "pfs/debby/backend/sqlite3/database.hpp"
#   include "pfs/debby/backend/sqlite3/statement.hpp"
#endif

namespace fs = pfs::filesystem;

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

template <typename Backend>
void check (pfs::filesystem::path const & db_path)
{
    using database_t = debby::relational_database<Backend>;

    if (fs::exists(db_path) && fs::is_regular_file(db_path))
        fs::remove(db_path);

    auto db = database_t::make(db_path);

    REQUIRE(db);

    db.remove_all();

    {
        auto stmt = db.prepare(fmt::format(CREATE_TABLE, TABLE_NAME));

        REQUIRE(stmt);
        auto result = stmt.exec();
        REQUIRE(result.is_done());
    }

    {
        auto stmt = db.prepare(fmt::format(INSERT, TABLE_NAME));

        REQUIRE(stmt);

        stmt.bind(":null", nullptr);
        stmt.bind(":bool", true);
        stmt.bind(":int8", std::numeric_limits<std::int8_t>::min());
        stmt.bind(":uint8", std::numeric_limits<std::uint8_t>::max());
        stmt.bind(":int16", std::numeric_limits<std::int16_t>::min());
        stmt.bind(":uint16", std::numeric_limits<std::uint16_t>::max());
        stmt.bind(":int32", std::numeric_limits<std::int32_t>::min());
        stmt.bind(":uint32", std::numeric_limits<std::uint32_t>::max());
        stmt.bind(":int64", std::numeric_limits<std::int64_t>::min());
        stmt.bind(":uint64", std::numeric_limits<std::uint64_t>::max());
        stmt.bind(":float", static_cast<float>(3.14159));
        stmt.bind(":double", static_cast<double>(3.14159));
        stmt.bind(":text", std::string{"Hello"});
        stmt.bind(":cstr", "World");

        auto result = stmt.exec();
        REQUIRE(result.is_done());
    }

    {
        auto stmt = db.prepare(fmt::format(SELECT_ALL, TABLE_NAME));

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
            {
                REQUIRE_EQ(result.template get_or<int>("unknown", -42), -42);
            }

            {
                // Column `null` is INTEGER but contains null value
                REQUIRE_EQ(result.template get<int *>("null"), nullptr);
                REQUIRE_EQ(result.template get<std::string *>("null"), nullptr);

#if PFS__EXCEPTIONS_ENABLED
                REQUIRE_THROWS(result.template get<int>("null"));
#endif
            }

            CHECK_EQ(result.template get<bool>("bool"), true);
            CHECK_EQ(result.template get<std::int8_t>("int8"), std::numeric_limits<std::int8_t>::min());
            CHECK_EQ(result.template get<std::uint8_t>("uint8"), std::numeric_limits<std::uint8_t>::max());
            CHECK_EQ(result.template get<std::int16_t>("int16"), std::numeric_limits<std::int16_t>::min());
            CHECK_EQ(result.template get<std::uint16_t>("uint16"), std::numeric_limits<std::uint16_t>::max());
            CHECK_EQ(result.template get<std::int32_t>("int32"), std::numeric_limits<std::int32_t>::min());
            CHECK_EQ(result.template get<std::uint32_t>("uint32"), std::numeric_limits<std::uint32_t>::max());
            CHECK_EQ(result.template get<std::int64_t>("int64"), std::numeric_limits<std::int64_t>::min());
            CHECK_EQ(result.template get<std::uint64_t>("uint64"), std::numeric_limits<std::uint64_t>::max());
            CHECK(std::abs(result.template get<float>("float") - static_cast<float>(3.14159)) < float{0.001});
            CHECK(std::abs(result.template get<double>("double") - static_cast<double>(3.14159)) < double(0.001));
            CHECK_EQ(result.template get<std::string>("text"), std::string{"Hello"});

            {
                bool b;
                result.template operator []("bool") >> b;
                CHECK_EQ(b, true);
            }

            {
                std::int8_t i8;
                result.template operator []("int8") >> i8;
                CHECK(i8 == std::numeric_limits<std::int8_t>::min());
            }

            {
                std::int64_t i64;
                result.template operator []("int64") >> i64;
                CHECK(i64 == std::numeric_limits<std::int64_t>::min());
            }

            {
                std::uint64_t u64;
                result.template operator []("uint64") >> u64;
                CHECK(u64 == std::numeric_limits<std::uint64_t>::max());
            }

            {
                float f;
                result.template operator []("float") >> f;
                CHECK(std::abs(f - static_cast<float>(3.14159)) < float{0.001});
            }

            {
                double f;
                result.template operator []("double") >> f;
                CHECK(std::abs(f - static_cast<double>(3.14159)) < double{0.001});
            }

            {
                std::string s;
                result.template operator []("text") >> s;
                CHECK(s == "Hello");
            }

            {
#if PFS__EXCEPTIONS_ENABLED
                // `null` value results throwing exception for `direct` variable
                int n;
                REQUIRE_THROWS((result.template operator []("null") >> n));
#endif

                pfs::optional<int> opt;

                // `null` value results nullopt
                result.template operator []("null") >> opt;
                REQUIRE_FALSE(opt.has_value());
            }

            {
#if PFS__EXCEPTIONS_ENABLED
                // Unknown column results throwing exception
                REQUIRE_THROWS((result.template operator []("unknown")));
#endif
            }

            result.next();
        }

        CHECK(result.is_done());
    }

    {
        for (int i = 0; i < 2; i++) {
            auto stmt = db.prepare(fmt::format(SELECT, TABLE_NAME), true);

            REQUIRE(stmt);

            stmt.bind(":int8", std::numeric_limits<std::int8_t>::min());

            auto result = stmt.exec();

            while (result.has_more())
                result.next();

            REQUIRE(result.is_done());
        }
    }
}

#if DEBBY__SQLITE3_ENABLED
TEST_CASE("sqlite3 statement") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3.db");
    check<debby::backend::sqlite3::database>(db_path);
}
#endif

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
#   include "pfs/debby/sqlite3.hpp"
#endif
//
#if DEBBY__PSQL_ENABLED
#   include "pfs/debby/psql.hpp"
#   include "psql_support.hpp"
#endif

namespace fs = pfs::filesystem;

namespace {
std::string TABLE_NAME {"test"};

std::string const CREATE_TABLE {
    "CREATE TABLE IF NOT EXISTS {} ("
        "null_field INTEGER"
        ", bool BOOLEAN"
        ", int8 INTEGER"
        ", uint8 INTEGER"
        ", int16 INTEGER"
        ", uint16 INTEGER"
        ", int32 INTEGER"
        ", uint32 INTEGER"
        ", int64 BIGINT"
        ", uint64 BIGINT"
        ", float REAL"
        ", double DOUBLE PRECISION"
        ", text TEXT"
        ", cstr TEXT)"
};

std::string const INSERT_SQLITE3 {
    "INSERT INTO {} (null_field, bool, int8, uint8"
        ", int16, uint16, int32, uint32"
        ", int64, uint64, float, double"
        ", text, cstr)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
};

std::string const INSERT_PSQL {
    "INSERT INTO {} (null_field, bool, int8, uint8"
        ", int16, uint16, int32, uint32"
        ", int64, uint64, float, double"
        ", text, cstr)"
    " VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14)"
};

std::string const SELECT_ALL {
    "SELECT * FROM {}"
};

std::string const SELECT_SQLITE3 {
    "SELECT * FROM {} WHERE int8 = ?"
};

std::string const SELECT_PSQL {
    "SELECT * FROM {} WHERE int8 = $1"
};

} // namespace

template <typename RelationalDatabaseType>
void check (RelationalDatabaseType & db, std::string const & insert_statement_format)
{
    db.remove_all();

    {
        auto stmt = db.prepare(fmt::format(CREATE_TABLE, TABLE_NAME));

        REQUIRE(stmt);
        auto result = stmt.exec();
        REQUIRE(result.is_done());
    }

    {
        auto stmt = db.prepare(fmt::format(insert_statement_format, TABLE_NAME));

        REQUIRE(stmt);

        // Not all backends support placeholders
        // stmt.bind(":null", nullptr);
        // stmt.bind(":bool", true);
        // stmt.bind(":int8", std::numeric_limits<std::int8_t>::min());
        // stmt.bind(":uint8", std::numeric_limits<std::uint8_t>::max());
        // stmt.bind(":int16", std::numeric_limits<std::int16_t>::min());
        // stmt.bind(":uint16", std::numeric_limits<std::uint16_t>::max());
        // stmt.bind(":int32", std::numeric_limits<std::int32_t>::min());
        // stmt.bind(":uint32", std::numeric_limits<std::uint32_t>::max());
        // stmt.bind(":int64", std::numeric_limits<std::int64_t>::min());
        // stmt.bind(":uint64", std::numeric_limits<std::uint64_t>::max());
        // stmt.bind(":float", static_cast<float>(3.14159));
        // stmt.bind(":double", static_cast<double>(3.14159));
        // stmt.bind(":text", std::string{"Hello"});
        // stmt.bind(":cstr", "World");

        stmt.bind(0, nullptr);
        stmt.bind(1, true);
        stmt.bind(2, std::numeric_limits<std::int8_t>::min());
        stmt.bind(3, std::numeric_limits<std::uint8_t>::max());
        stmt.bind(4, std::numeric_limits<std::int16_t>::min());
        stmt.bind(5, std::numeric_limits<std::uint16_t>::max());
        stmt.bind(6, std::numeric_limits<std::int32_t>::min());
        stmt.bind(7, std::numeric_limits<std::uint32_t>::max());
        stmt.bind(8, std::numeric_limits<std::int64_t>::min());
        stmt.bind(9, std::numeric_limits<std::uint64_t>::max());
        stmt.bind(10, static_cast<float>(3.14159));
        stmt.bind(11, static_cast<double>(3.14159));
        stmt.bind(12, std::string{"Hello"});
        stmt.bind(13, "World");

        auto result = stmt.exec();
        REQUIRE(result.is_done());
        REQUIRE_EQ(db.rows_count(TABLE_NAME), 1);
    }

    {
        auto stmt = db.prepare(fmt::format(SELECT_ALL, TABLE_NAME));

        REQUIRE(stmt);

        auto result = stmt.exec();
        REQUIRE(result.has_more());
        REQUIRE_FALSE(result.is_done());

        CHECK_EQ(result.column_count(), 14);

        CHECK_EQ(result.column_name(-1), std::string{});
        CHECK_EQ(result.column_name(14), std::string{});

        CHECK_EQ(result.column_name(0), std::string{"null_field"});
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
                REQUIRE_THROWS_AS(result.template get_or<int>("unknown", -42), debby::error);
            }

            CHECK_THROWS_AS(result.template get<int>("null_field"), debby::error);
            CHECK_EQ(result.template get_or<int>("null_field", 0), 0);
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

            result.next();
        }

        CHECK(result.is_done());

        REQUIRE_EQ(db.rows_count(TABLE_NAME), 1);
        db.clear(TABLE_NAME);
        REQUIRE_EQ(db.rows_count(TABLE_NAME), 0);
    }
}

template <typename RelationalDatabaseType>
void prepared_select (RelationalDatabaseType & db, std::string const & sql)
{
    for (int i = 0; i < 2; i++) {
        auto stmt = db.prepare(fmt::format(sql, TABLE_NAME), true);

        REQUIRE(stmt);

        stmt.bind(0, std::numeric_limits<std::int8_t>::min());

        auto result = stmt.exec();

        while (result.has_more())
            result.next();

        REQUIRE(result.is_done());
    }
}

#if DEBBY__SQLITE3_ENABLED
TEST_CASE("sqlite3") {
    using database_t = debby::relational_database<debby::backend_enum::sqlite3>;

    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3.db");
    database_t::wipe(db_path);

    auto db = database_t::make(db_path);

    REQUIRE(db);

    check(db, INSERT_SQLITE3);
    prepared_select(db, SELECT_SQLITE3);

    database_t::wipe(db_path);
}
#endif

#if DEBBY__PSQL_ENABLED
TEST_CASE("PostgreSQL") {
    using database_t = debby::relational_database<debby::backend_enum::psql>;

    auto conninfo = psql_conninfo();
    auto db = database_t::make(conninfo.cbegin(), conninfo.cend());

    if (!db) {
        MESSAGE(preconditions_notice());
    }

    REQUIRE(db);

    check(db, INSERT_PSQL);
    prepared_select(db, SELECT_PSQL);
}
#endif

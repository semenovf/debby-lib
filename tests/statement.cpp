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

#include "pfs/debby/time_affinity.hpp"
#include "pfs/debby/universal_id_affinity.hpp"

namespace fs = pfs::filesystem;

namespace {
std::string TABLE_NAME {"test"};

std::string const CREATE_TABLE {
    "CREATE TABLE IF NOT EXISTS {} ("
        "null_field INTEGER"
        ", bool BOOLEAN"
        ", int8min INTEGER"
        ", uint8max INTEGER"
        ", int16min INTEGER"
        ", uint16max INTEGER"
        ", int32min INTEGER"
        ", uint32max BIGINT"
        ", int64min BIGINT"
        ", uint64max BIGINT"
        ", int INTEGER"
        ", uint INTEGER"
        ", float REAL"
        ", double DOUBLE PRECISION"
        ", text TEXT"
        ", cstr TEXT"
        ", uid TEXT"
        ", utc_time INTEGER"
        ", local_time INTEGER"
    ")"
};

std::string const INSERT_SQLITE3 {
    "INSERT INTO {} (null_field, bool, int8min, uint8max"
        ", int16min, uint16max, int32min, uint32max"
        ", int64min, uint64max, int, uint"
        ", float, double, text, cstr, uid, utc_time, local_time)"
    " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
};

std::string const INSERT_PSQL {
    "INSERT INTO {} (null_field, bool, int8min, uint8max"
        ", int16min, uint16max, int32min, uint32max"
        ", int64min, uint64max, int, uint"
        ", float, double, text, cstr, uid, utc_time, local_time)"
    " VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19)"
};

std::string const SELECT_ALL {
    "SELECT * FROM {}"
};

std::string const SELECT_SQLITE3 {
    "SELECT * FROM {} WHERE int8min = ?"
};

std::string const SELECT_PSQL {
    "SELECT * FROM {} WHERE int8min = $1"
};

} // namespace

template <typename RelationalDatabaseType>
void check (RelationalDatabaseType & db, std::string const & insert_statement_format)
{
    db.remove_all();
    auto uid = pfs::generate_uuid();
    auto utc_time = pfs::utc_time {pfs::utc_time::now().to_millis()};
    auto local_time = pfs::local_time {pfs::local_time::now().to_millis()};

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

        stmt.bind(1, nullptr);
        stmt.bind(2, true);
        stmt.bind(3, std::numeric_limits<std::int8_t>::min());
        stmt.bind(4, std::numeric_limits<std::uint8_t>::max());
        stmt.bind(5, std::numeric_limits<std::int16_t>::min());
        stmt.bind(6, std::numeric_limits<std::uint16_t>::max());
        stmt.bind(7, std::numeric_limits<std::int32_t>::min());
        stmt.bind(8, std::numeric_limits<std::uint32_t>::max());
        stmt.bind(9, std::numeric_limits<std::int64_t>::min());
        stmt.bind(10, std::numeric_limits<std::uint64_t>::max());
        stmt.bind(11, -42);
        stmt.bind(12, 42);
        stmt.bind(13, static_cast<float>(3.14159));
        stmt.bind(14, static_cast<double>(3.14159));
        stmt.bind(15, std::string{"Hello"});
        stmt.bind(16, "World");

        stmt.bind(17, uid);
        stmt.bind(18, utc_time);
        stmt.bind(19, local_time);

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

        CHECK_EQ(result.column_count(), 19);

        CHECK_EQ(result.column_name(-1), std::string{});
        CHECK_EQ(result.column_name(20), std::string{});

        CHECK_EQ(result.column_name(1), std::string{"null_field"});
        CHECK_EQ(result.column_name(2), std::string{"bool"});
        CHECK_EQ(result.column_name(3), std::string{"int8min"});
        CHECK_EQ(result.column_name(4), std::string{"uint8max"});
        CHECK_EQ(result.column_name(5), std::string{"int16min"});
        CHECK_EQ(result.column_name(6), std::string{"uint16max"});
        CHECK_EQ(result.column_name(7), std::string{"int32min"});
        CHECK_EQ(result.column_name(8), std::string{"uint32max"});
        CHECK_EQ(result.column_name(9), std::string{"int64min"});
        CHECK_EQ(result.column_name(10), std::string{"uint64max"});
        CHECK_EQ(result.column_name(11), std::string{"int"});
        CHECK_EQ(result.column_name(12), std::string{"uint"});
        CHECK_EQ(result.column_name(13), std::string{"float"});
        CHECK_EQ(result.column_name(14), std::string{"double"});
        CHECK_EQ(result.column_name(15), std::string{"text"});
        CHECK_EQ(result.column_name(16), std::string{"cstr"});

        CHECK_EQ(result.column_name(17), std::string{"uid"});
        CHECK_EQ(result.column_name(18), std::string{"utc_time"});
        CHECK_EQ(result.column_name(19), std::string{"local_time"});

        while (result.has_more()) {
            {
                REQUIRE_THROWS_AS(result.template get_or<int>("unknown", -42), debby::error);
            }

            CHECK_EQ(result.template get<int>("null_field"), pfs::nullopt);
            CHECK_EQ(result.template get_or<int>("null_field", 0), 0);
            CHECK_EQ(result.template get_or<bool>("bool", false), true);

            CHECK_EQ(result.template get_or<std::int8_t>("int8min", 0), std::numeric_limits<std::int8_t>::min());
            CHECK_EQ(result.template get_or<std::uint8_t>("uint8max", 0), std::numeric_limits<std::uint8_t>::max());
            CHECK_EQ(result.template get_or<std::int16_t>("int16min", 0), std::numeric_limits<std::int16_t>::min());
            CHECK_EQ(result.template get_or<std::uint16_t>("uint16max", 0), std::numeric_limits<std::uint16_t>::max());
            CHECK_EQ(result.template get_or<std::int32_t>("int32min", 0), std::numeric_limits<std::int32_t>::min());
            CHECK_EQ(result.template get_or<std::uint32_t>("uint32max", 0), std::numeric_limits<std::uint32_t>::max());
            CHECK_EQ(result.template get_or<std::int64_t>("int64min", 0), std::numeric_limits<std::int64_t>::min());
            CHECK_EQ(result.template get_or<std::uint64_t>("uint64max", 0), std::numeric_limits<std::uint64_t>::max());

            CHECK_EQ(result.template get_or<std::int8_t>("int", 0), -42);
            CHECK_EQ(result.template get_or<std::uint8_t>("uint", 0), 42);
            CHECK_EQ(result.template get_or<std::int16_t>("int", 0), -42);
            CHECK_EQ(result.template get_or<std::uint16_t>("uint", 0), 42);
            CHECK_EQ(result.template get_or<std::int32_t>("int", 0), -42);
            CHECK_EQ(result.template get_or<std::uint32_t>("uint", 0), 42);
            CHECK_EQ(result.template get_or<std::int64_t>("int", 0), -42);
            CHECK_EQ(result.template get_or<std::uint64_t>("uint", 0), 42);

            CHECK_EQ(result.template get_or<float>("float", 0), float{3.14159});
            CHECK_EQ(result.template get_or<double>("double", 0), double{3.14159});
            CHECK_EQ(result.template get_or<std::string>("text", ""), std::string{"Hello"});

            CHECK_EQ(result.template get_or<pfs::universal_id>("uid", pfs::universal_id{}), uid);
            CHECK_EQ(result.template get<pfs::universal_id>(17), uid);

            CHECK_EQ(result.template get<pfs::utc_time>("utc_time"), utc_time);
            CHECK_EQ(result.template get<pfs::utc_time>(18), utc_time);
            CHECK_EQ(result.template get<pfs::local_time>("local_time"), local_time);
            CHECK_EQ(result.template get<pfs::local_time>(19), local_time);

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
        auto stmt = db.prepare(fmt::format(sql, TABLE_NAME));

        REQUIRE(stmt);

        stmt.bind(1, std::numeric_limits<std::int8_t>::min());

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
    debby::error err;
    using database_t = debby::relational_database<debby::backend_enum::psql>;

    auto conninfo = psql_conninfo();
    auto db = database_t::make(conninfo.cbegin(), conninfo.cend(), & err);

    if (!db) {
        WARN(db);
        MESSAGE(err.what());
        MESSAGE(preconditions_notice());
        return;
    }

    REQUIRE(db);

    check(db, INSERT_PSQL);
    prepared_select(db, SELECT_PSQL);
}
#endif

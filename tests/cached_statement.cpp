////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.01 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include "pfs/fmt.hpp"
#include "pfs/debby/sqlite3/database.hpp"
#include "pfs/debby/sqlite3/statement.hpp"
#include <vector>

using database_t = debby::sqlite3::database;

namespace {

std::string const CREATE_TABLE {
    "CREATE TABLE IF NOT EXISTS `{}` ("
        "`id` INTEGER"
        ", `string` TEXT"
        ", `blob` BLOB)"
};

std::string const INSERT {
    "INSERT INTO `{}` (`id`, `string`, `blob`) VALUES (:id, :string, :blob)"
};

std::string const SELECT {
    "SELECT * FROM `{}` WHERE `id` = :id"
};

std::string const SELECT_ALL {
    "SELECT * FROM `{}`"
};

constexpr int MAX_ITERATIONS = 10;
constexpr int MIN_EPOCH_ITERATIONS = 2;
} // namespace

void benchmark_insert_op (database_t * db, std::string const & table_name, bool cache)
{
    REQUIRE(db->begin());

    for (int i = 0; i < MAX_ITERATIONS; i++) {
        auto stmt = db->prepare(fmt::format(INSERT, table_name), cache);
        REQUIRE(stmt);

        bool success = stmt.bind(":id", i)
            && stmt.bind(":string", std::string{"Hello"})
            && stmt.bind(":blob", std::vector<std::uint8_t>{1,2,3,4,5,6,7,9});

        REQUIRE(success);
        auto result = stmt.exec();
        REQUIRE(result.is_done());
    }

    REQUIRE(db->commit());
}

void benchmark_select_all_op (database_t * db, std::string const & table_name, bool cache)
{
    REQUIRE(db->begin());

    for (int i = 0; i < MAX_ITERATIONS; i++) {
        auto stmt = db->prepare(fmt::format(SELECT_ALL, table_name), cache);
        REQUIRE(stmt);
        auto result = stmt.exec();
    }

    REQUIRE(db->commit());
}

void benchmark_select_op (database_t * db, std::string const & table_name, bool cache)
{
    REQUIRE(db->begin());

    for (int i = 0; i < MAX_ITERATIONS; i++) {
        auto stmt = db->prepare(fmt::format(SELECT, table_name), cache);
        REQUIRE(stmt);
        REQUIRE_NOTHROW(stmt.bind(":id", i));
        auto result = stmt.exec();
    }

    REQUIRE(db->commit());
}

void benchmark_initialize (database_t * db, std::string const & table_name)
{
    // Do not cache this statement ----------------------------------v
    auto stmt = db->prepare(fmt::format(CREATE_TABLE, table_name), false);

    REQUIRE(stmt);
    auto result = stmt.exec();
    REQUIRE(result.is_done());
}

void benchmark_insert (database_t * db, std::string const & table_name, bool cache)
{
    ankerl::nanobench::Bench()
        .minEpochIterations(MIN_EPOCH_ITERATIONS)
        .title("insert")
        .name(table_name)
        .run([db, table_name, cache] {
            benchmark_insert_op(db, table_name, cache);
        });
}

void benchmark_select_all (database_t * db, std::string const & table_name, bool cache)
{
    ankerl::nanobench::Bench()
        .minEpochIterations(MIN_EPOCH_ITERATIONS)
        .title("select_all")
        .name(table_name)
        .run([db, table_name, cache] {
            benchmark_select_all_op(db, table_name, cache);
        });
}

void benchmark_select (database_t * db, std::string const & table_name, bool cache)
{
    ankerl::nanobench::Bench()
        .minEpochIterations(MIN_EPOCH_ITERATIONS)
        .title("select")
        .name(table_name)
        .run([db, table_name, cache] {
            benchmark_select_op(db, table_name, cache);
        });
}

void do_benchmarks (database_t * db
    , std::string const & noncached_table_name
    , std::string const & cached_table_name)
{
    benchmark_initialize(db, noncached_table_name);
    benchmark_initialize(db, cached_table_name);

    benchmark_insert(db, noncached_table_name, false);
    benchmark_insert(db, cached_table_name, true);

    benchmark_select_all(db, noncached_table_name, false);
    benchmark_select_all(db, cached_table_name, true);

    benchmark_select(db, noncached_table_name, false);
    benchmark_select(db, cached_table_name, true);
}

//==============================================================================
// Ubuntu 20.04.3 LTS
// Intel(R) Core(TM) i5-2500K CPU @ 3.30GHz
// RAM 16 Gb
// |               ns/op |                op/s |    err% |     total | insert
// |--------------------:|--------------------:|--------:|----------:|:-------
// |      133,650,148.50 |                7.48 |    0.1% |      2.88 | `noncached`
// |      133,670,566.50 |                7.48 |    0.4% |      4.14 | `cached`
//
// |               ns/op |                op/s |    err% |     total | select_all
// |--------------------:|--------------------:|--------:|----------:|:-----------
// |          348,676.50 |            2,867.99 |    0.8% |      0.01 | `noncached`
// |          130,788.50 |            7,645.93 |    0.3% |      0.00 | `cached`
//
// |               ns/op |                op/s |    err% |     total | select
// |--------------------:|--------------------:|--------:|----------:|:-------
// |          218,973.00 |            4,566.77 |    0.2% |      0.01 | `noncached`
// |           78,246.50 |           12,780.12 |    0.6% |      0.00 | `cached`
//==============================================================================
TEST_CASE("benchmark") {
    namespace fs = pfs::filesystem;

    auto db_path = fs::temp_directory_path() / "benchmark.db";

    database_t db {db_path};

    REQUIRE(db);
    REQUIRE(db.remove_all());

    do_benchmarks(& db, "noncached", "cached");
}


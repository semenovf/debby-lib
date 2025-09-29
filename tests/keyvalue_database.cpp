////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2025 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2022.03.12 Refactored.
//      2025.09.29 Added tests for blob, universal_id, utc_time, local_time.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <limits>
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/settings.hpp"
#include <pfs/filesystem.hpp>

#if DEBBY__MAP_ENABLED
#   include "pfs/debby/in_memory.hpp"
#endif

#if DEBBY__UNORDERED_MAP_ENABLED
#   include "pfs/debby/in_memory.hpp"
#endif

#if DEBBY__SQLITE3_ENABLED
#   include "pfs/debby/sqlite3.hpp"
#endif

#if DEBBY__LMDB_ENABLED
#   include "pfs/debby/lmdb.hpp"
#endif

#if DEBBY__MDBX_ENABLED
#   include "pfs/debby/mdbx.hpp"
#endif

#if DEBBY__ROCKSDB_ENABLED
#   include "pfs/debby/rocksdb.hpp"
#endif

#if DEBBY__PSQL_ENABLED
#   include "pfs/debby/psql.hpp"
#   include "psql_support.hpp"
#endif

#include "pfs/debby/time_affinity.hpp"
#include "pfs/debby/universal_id_affinity.hpp"

namespace fs = pfs::filesystem;

template <debby::backend_enum Backend>
void check_keyvalue_database (debby::keyvalue_database<Backend> & db)
{
    try {
        REQUIRE(db);

        db.set("bool", true);
        db.set("char", 'W');
        db.set("signed char", static_cast<signed char>(-42));
        db.set("unsigned char", static_cast<unsigned char>(42));
        db.set("short", static_cast<short>(-42));
        db.set("unsigned short", static_cast<unsigned short>(42));
        db.set("int", -42);
        db.set("unsigned int", static_cast<unsigned int>(42));
        db.set("long", static_cast<long>(-42));
        db.set("unsigned long", static_cast<unsigned long>(42));
        db.set("long long", static_cast<long long>(-42));
        db.set("unsigned long long", static_cast<unsigned long long>(42));
        db.set("float", static_cast<float>(3.14159));
        db.set("double", static_cast<double>(3.14159));
        db.set("text"  , std::string{"Hello"});
        db.set("cstr"  , "World");
        db.set("empty" , std::string{""});

        std::string blob = "This is a blob";
        db.set("blob", blob.data(), blob.size());

        auto uid = pfs::generate_uuid();
        db.set("uid", uid);

        // Cast time to the milliseconds precision for properly comparison
        auto utc_time = pfs::utc_time {pfs::utc_time::now().to_millis()};
        db.set("utc_time", utc_time);

        // Cast time to the milliseconds precision for properly comparison
        auto local_time = pfs::local_time {pfs::local_time::now().to_millis()};
        db.set("local_time", local_time);

        REQUIRE_EQ(db.template get_or<int>("unknown", -1), -1);

        REQUIRE_EQ(db.template get<bool>("bool"), true);
        REQUIRE_EQ(db.template get<char>("char"), 'W');
        REQUIRE_EQ(db.template get<signed char>("signed char"), static_cast<signed char>(-42));
        REQUIRE_EQ(db.template get<unsigned char>("unsigned char"), static_cast<unsigned char>(42));
        REQUIRE_EQ(db.template get<short>("short"), -42);
        REQUIRE_EQ(db.template get<unsigned short>("unsigned short"), 42);
        REQUIRE_EQ(db.template get<int>("int"), -42);
        REQUIRE_EQ(db.template get<unsigned int>("unsigned int"), 42);
        REQUIRE_EQ(db.template get<long>("long"), -42);
        REQUIRE_EQ(db.template get<unsigned long>("unsigned long"), 42);
        REQUIRE_EQ(db.template get<long long>("long long"), -42);
        REQUIRE_EQ(db.template get<unsigned long long>("unsigned long long"), 42);
        REQUIRE_EQ(db.template get<float>("float"), static_cast<float>(3.14159));
        REQUIRE_EQ(db.template get<double>("double"), static_cast<double>(3.14159));

        REQUIRE_EQ(db.template get<std::string>("text"), std::string{"Hello"});
        REQUIRE_EQ(db.template get<std::string>("cstr"), std::string{"World"});
        REQUIRE(db.template get<std::string>("empty").empty());

        REQUIRE_EQ(db.template get<std::string>("blob"), blob);

        REQUIRE_EQ(db.template get<pfs::universal_id>("uid"), uid);
        REQUIRE_EQ(db.template get<pfs::utc_time>("utc_time"), utc_time);
        REQUIRE_EQ(db.template get<pfs::local_time>("local_time"), local_time);

        db.remove("text");
        REQUIRE(db.template get_or<std::string>("text", "").empty());
    } catch (debby::error ex) {
        REQUIRE_MESSAGE(false, ex.what());
    }
}

template <debby::backend_enum Backend>
void check_settings (debby::settings<Backend> & db)
{
    try {
        REQUIRE(db);

        db.set("bool.value"  , true);
        db.set("char.value"  , 'W');
        db.set("int8.value"  , std::numeric_limits<std::int8_t>::min());
        db.set("uint8.value" , std::numeric_limits<std::uint8_t>::max());
        db.set("int16.value" , std::numeric_limits<std::int16_t>::min());
        db.set("uint16.value", std::numeric_limits<std::uint16_t>::max());
        db.set("int32.value" , std::numeric_limits<std::int32_t>::min());
        db.set("uint32.value", std::numeric_limits<std::uint32_t>::max());
        db.set("int64.value" , std::numeric_limits<std::int64_t>::min());
        db.set("uint64.value", std::numeric_limits<std::uint64_t>::max());
        db.set("float.value" , static_cast<float>(3.14159));
        db.set("double.value", static_cast<double>(3.14159));
        db.set("text.value"  , std::string{"Hello"});
        db.set("empty.value" , std::string{""});
        db.set("cstr.value"  , "World");

        REQUIRE_EQ(db.template get<int>("unknown", -1), -1);

        REQUIRE_EQ(db.template get<bool>("bool.value")            , true);
        REQUIRE_EQ(db.template get<char>("char.value")            , 'W');
        REQUIRE_EQ(db.template get<std::int8_t>("int8.value")     , std::numeric_limits<std::int8_t>::min());
        REQUIRE_EQ(db.template get<std::uint8_t>("uint8.value")   , std::numeric_limits<std::uint8_t>::max());
        REQUIRE_EQ(db.template get<std::int16_t>("int16.value")   , std::numeric_limits<std::int16_t>::min());
        REQUIRE_EQ(db.template get<std::uint16_t>("uint16.value") , std::numeric_limits<std::uint16_t>::max());
        REQUIRE_EQ(db.template get<std::int32_t>("int32.value")   , std::numeric_limits<std::int32_t>::min());
        REQUIRE_EQ(db.template get<std::uint32_t>("uint32.value") , std::numeric_limits<std::uint32_t>::max());
        REQUIRE_EQ(db.template get<std::int64_t>("int64.value")   , std::numeric_limits<std::int64_t>::min());
        REQUIRE_EQ(db.template get<std::uint64_t>("uint64.value") , std::numeric_limits<std::uint64_t>::max());
        REQUIRE_EQ(db.template get<float>("float.value"), static_cast<float>(3.14159));
        REQUIRE_EQ(db.template get<double>("double.value"), static_cast<double>(3.14159));

        REQUIRE_EQ(db.template get<std::string>("text.value"), std::string{"Hello"});
        REQUIRE(db.template get<std::string>("empty.value").empty());

        db.remove("text.value");

        REQUIRE(db.template get<std::string>("text.value", "").empty());

        db.template take<std::string>("text.value", "Hello, World!");
        REQUIRE_EQ(db.template get<std::string>("text.value", ""), std::string{"Hello, World!"});
    } catch (debby::error ex) {
        REQUIRE_MESSAGE(false, ex.what());
    }
}

template <debby::backend_enum Backend>
void check (debby::keyvalue_database<Backend> && db)
{
    using settings_t = debby::settings<Backend>;

    // Create and destruct database
    {
        db.clear();
        check_keyvalue_database(db);

        settings_t settings {std::move(db)};
        check_settings(settings);
    }
}

#if DEBBY__MAP_ENABLED
TEST_CASE("in-memory thread unsafe map set/get") {
    using database_t = debby::keyvalue_database<debby::backend_enum::map_st>;
    auto db = database_t::make();
    check(std::move(db));
}

TEST_CASE("in-memory thread safe map set/get") {
    using database_t = debby::keyvalue_database<debby::backend_enum::map_mt>;
    auto db = database_t::make();
    check(std::move(db));
}
#endif

#if DEBBY__UNORDERED_MAP_ENABLED
TEST_CASE("in-memory thread unsafe unordered_map set/get") {
    using database_t = debby::keyvalue_database<debby::backend_enum::unordered_map_st>;
    auto db = database_t::make();
    check(std::move(db));
}

TEST_CASE("in-memory thread safe unordered_map set/get") {
    using database_t = debby::keyvalue_database<debby::backend_enum::unordered_map_mt>;
    auto db = database_t::make();
    check(std::move(db));
}
#endif

#if DEBBY__LMDB_ENABLED
TEST_CASE("lmdb set/get") {
    using database_t = debby::keyvalue_database<debby::backend_enum::lmdb>;
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-lmdb-kv.db");
    auto db = database_t::make(db_path, true);
    db.clear();
    check(std::move(db));
}
#endif

#if DEBBY__MDBX_ENABLED
TEST_CASE("dbmx set/get") {
    using database_t = debby::keyvalue_database<debby::backend_enum::mdbx>;
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-mdbx-kv.db");
    auto db = database_t::make(db_path, true);
    db.clear();
    check(std::move(db));
}
#endif

#if DEBBY__ROCKSDB_ENABLED
TEST_CASE("rocksdb set/get") {
    using database_t = debby::keyvalue_database<debby::backend_enum::rocksdb>;
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-rocksdb-kv.db");
    auto db = database_t::make(db_path, true);
    db.clear();
    check(std::move(db));
}
#endif

#if DEBBY__SQLITE3_ENABLED
 TEST_CASE("sqlite3 set/get") {
    using database_t = debby::keyvalue_database<debby::backend_enum::sqlite3>;
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3-kv.db");
    auto db = database_t::make(db_path, "test-kv", true);
    db.clear();
    check(std::move(db));
}
#endif

#if DEBBY__PSQL_ENABLED
TEST_CASE("PostgreSQL set/get") {
    debby::error err;
    using database_t = debby::keyvalue_database<debby::backend_enum::psql>;
    auto conninfo = psql_conninfo();
    auto db = database_t::make(conninfo.cbegin(), conninfo.cend(), "debby-kv", & err);

    if (!db) {
        WARN(db);
        MESSAGE(err.what());
        MESSAGE(preconditions_notice());
        return;
    }

    REQUIRE(db);

    db.clear();
    check(std::move(db));
}
#endif

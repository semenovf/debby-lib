////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.07 Initial version.
//      2022.03.12 Refactored.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/filesystem.hpp"
#include <limits>
#include "pfs/debby/keyvalue_database.hpp"
#include "pfs/debby/settings.hpp"
#include "pfs/debby/backend/in_memory/map.hpp"
#include "pfs/debby/backend/in_memory/unordered_map.hpp"
#include "pfs/debby/backend/sqlite3/database.hpp"

#if DEBBY__LIBMDBX_ENABLED
#   include "pfs/debby/backend/libmdbx/database.hpp"
#endif

#if DEBBY__ROCKSDB_ENABLED
#   include "pfs/debby/backend/rocksdb/database.hpp"
#endif

namespace fs = pfs::filesystem;

template <typename Backend>
void check_keyvalue_database (debby::keyvalue_database<Backend> & db)
{
    try {
        REQUIRE(db);

        db.set("bool"  , true);
        db.set("int8"  , std::numeric_limits<std::int8_t>::min());
        db.set("uint8" , std::numeric_limits<std::uint8_t>::max());
        db.set("int16" , std::numeric_limits<std::int16_t>::min());
        db.set("uint16", std::numeric_limits<std::uint16_t>::max());
        db.set("int32" , std::numeric_limits<std::int32_t>::min());
        db.set("uint32", std::numeric_limits<std::uint32_t>::max());
        db.set("int64" , std::numeric_limits<std::int64_t>::min());
        db.set("uint64", std::numeric_limits<std::uint64_t>::max());
        db.set("float" , static_cast<float>(3.14159));
        db.set("double", static_cast<double>(3.14159));
        db.set("text"  , std::string{"Hello"});
        db.set("empty" , std::string{""});
        db.set("cstr"  , "World");

        REQUIRE_EQ(db.template get_or<int>("unknown", -1), -1);

        REQUIRE_EQ(db.template get<bool>("bool")            , true);
        REQUIRE_EQ(db.template get<std::int8_t>("int8")     , std::numeric_limits<std::int8_t>::min());
        REQUIRE_EQ(db.template get<std::uint8_t>("uint8")   , std::numeric_limits<std::uint8_t>::max());
        REQUIRE_EQ(db.template get<std::int16_t>("int16")   , std::numeric_limits<std::int16_t>::min());
        REQUIRE_EQ(db.template get<std::uint16_t>("uint16") , std::numeric_limits<std::uint16_t>::max());
        REQUIRE_EQ(db.template get<std::int32_t>("int32")   , std::numeric_limits<std::int32_t>::min());
        REQUIRE_EQ(db.template get<std::uint32_t>("uint32") , std::numeric_limits<std::uint32_t>::max());
        REQUIRE_EQ(db.template get<std::int64_t>("int64")   , std::numeric_limits<std::int64_t>::min());
        REQUIRE_EQ(db.template get<std::uint64_t>("uint64") , std::numeric_limits<std::uint64_t>::max());
        REQUIRE(std::abs(db.template get<float>("float") - static_cast<float>(3.14159)) < float{0.001});
        REQUIRE(std::abs(db.template get<double>("double") - static_cast<double>(3.14159)) < double(0.001));

        REQUIRE_EQ(db.template get<std::string>("text"), std::string{"Hello"});
        REQUIRE(db.template get<std::string>("empty").empty());

        db.remove("text");
        REQUIRE(db.template get_or<std::string>("text", "").empty());
    } catch (debby::error ex) {
        REQUIRE_MESSAGE(false, ex.what());
    }
}

template <typename Backend>
void check_settings (debby::settings<Backend> & db)
{
    try {
        REQUIRE(db);

        db.set("bool.value"  , true);
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
        REQUIRE_EQ(db.template get<std::int8_t>("int8.value")     , std::numeric_limits<std::int8_t>::min());
        REQUIRE_EQ(db.template get<std::uint8_t>("uint8.value")   , std::numeric_limits<std::uint8_t>::max());
        REQUIRE_EQ(db.template get<std::int16_t>("int16.value")   , std::numeric_limits<std::int16_t>::min());
        REQUIRE_EQ(db.template get<std::uint16_t>("uint16.value") , std::numeric_limits<std::uint16_t>::max());
        REQUIRE_EQ(db.template get<std::int32_t>("int32.value")   , std::numeric_limits<std::int32_t>::min());
        REQUIRE_EQ(db.template get<std::uint32_t>("uint32.value") , std::numeric_limits<std::uint32_t>::max());
        REQUIRE_EQ(db.template get<std::int64_t>("int64.value")   , std::numeric_limits<std::int64_t>::min());
        REQUIRE_EQ(db.template get<std::uint64_t>("uint64.value") , std::numeric_limits<std::uint64_t>::max());
        REQUIRE(std::abs(db.template get<float>("float.value") - static_cast<float>(3.14159)) < float{0.001});
        REQUIRE(std::abs(db.template get<double>("double.value") - static_cast<double>(3.14159)) < double(0.001));

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

template <typename Backend>
void check_persistance_storage (pfs::filesystem::path const & db_path)
{
    using database_t = debby::keyvalue_database<Backend>;
    using settings_t = debby::settings<Backend>;

    if (fs::exists(db_path))
        fs::remove_all(db_path);

    auto db = database_t::make(db_path, true);
    check_keyvalue_database(db);

    settings_t settings {std::move(db)};
    check_settings(settings);

    // In Windows database must be closed/destructed before to avoid exception:
    // "The process cannot access the file because it is being used by another process"
    if (fs::exists(db_path))
        fs::remove_all(db_path);
}

TEST_CASE("in-memory thread unsafe map set/get") {
    using database_t = debby::keyvalue_database<debby::backend::in_memory::map_st>;
    using settings_t = debby::settings<debby::backend::in_memory::map_st>;
    auto db = database_t::make();
    check_keyvalue_database(db);
}

TEST_CASE("in-memory thread unsafe unordered_map set/get") {
    using database_t = debby::keyvalue_database<debby::backend::in_memory::unordered_map_st>;
    using settings_t = debby::settings<debby::backend::in_memory::unordered_map_st>;
    auto db = database_t::make();
    check_keyvalue_database(db);
}

TEST_CASE("in-memory thread safe map set/get") {
    using database_t = debby::keyvalue_database<debby::backend::in_memory::map_mt>;
    using settings_t = debby::settings<debby::backend::in_memory::map_mt>;
    auto db = database_t::make();
    check_keyvalue_database(db);
}

TEST_CASE("in-memory thread safe unordered_map set/get") {
    using database_t = debby::keyvalue_database<debby::backend::in_memory::unordered_map_mt>;
    using settings_t = debby::settings<debby::backend::in_memory::unordered_map_mt>;
    auto db = database_t::make();
    check_keyvalue_database(db);
}

TEST_CASE("sqlite3 set/get") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3-kv.db");
    check_persistance_storage<debby::backend::sqlite3::database>(db_path);
}

#if DEBBY__LIBMDBX_ENABLED
TEST_CASE("libdbmx set/get") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-libmdbx-kv.db");
    check_persistance_storage<debby::backend::libmdbx::database>(db_path);
}
#endif

#if DEBBY__ROCKSDB_ENABLED
TEST_CASE("rocksdb set/get") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-rocksdb-kv.db");
    check_persistance_storage<debby::backend::rocksdb::database>(db_path);
}
#endif

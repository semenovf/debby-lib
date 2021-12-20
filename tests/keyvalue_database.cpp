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
#include <limits>

#if DEBBY__ROCKSDB_ENABLED
#   include "pfs/debby/rocksdb/database.hpp"
#endif

namespace fs = pfs::filesystem;

template <typename T>
void check_set_get (pfs::filesystem::path const & db_path)
{
    using database_t = T;

    if (fs::exists(db_path) && fs::is_regular_file(db_path))
        fs::remove(db_path);

    database_t db {db_path};

    REQUIRE(db);

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
    REQUIRE(db.set("empty" , std::string{""}));
    REQUIRE(db.set("cstr"  , "World"));

    REQUIRE_NOTHROW(db.template get<int>("unknown"));
    REQUIRE_FALSE(db.template get<int>("unknown").has_value());

    REQUIRE_EQ(db.template get<bool>("bool")            , true);
    REQUIRE_EQ(db.template get<std::int8_t>("int8")     , std::numeric_limits<std::int8_t>::min());
    REQUIRE_EQ(db.template get<std::uint8_t>("uint8")   , std::numeric_limits<std::uint8_t>::max());
    REQUIRE_EQ(db.template get<std::int16_t>("int16")   , std::numeric_limits<std::int16_t>::min());
    REQUIRE_EQ(db.template get<std::uint16_t>("uint16") , std::numeric_limits<std::uint16_t>::max());
    REQUIRE_EQ(db.template get<std::int32_t>("int32")   , std::numeric_limits<std::int32_t>::min());
    REQUIRE_EQ(db.template get<std::uint32_t>("uint32") , std::numeric_limits<std::uint32_t>::max());
    REQUIRE_EQ(db.template get<std::int64_t>("int64")   , std::numeric_limits<std::int64_t>::min());
    REQUIRE_EQ(db.template get<std::uint64_t>("uint64") , std::numeric_limits<std::uint64_t>::max());
    REQUIRE(std::abs(*db.template get<float>("float") - static_cast<float>(3.14159)) < float{0.001});
    REQUIRE(std::abs(*db.template get<double>("double") - static_cast<double>(3.14159)) < double(0.001));

    REQUIRE_EQ(*db.template get<std::string>("text"), std::string{"Hello"});
    REQUIRE(db.template get<std::string>("empty")->empty());

    REQUIRE(db.remove("text"));
    REQUIRE_FALSE(db.template get<std::string>("text").has_value());
}

#if DEBBY__ROCKSDB_ENABLED
TEST_CASE("rocksdb set/get") {
    auto db_path = fs::temp_directory_path() / PFS_PLATFORM_LITERAL("debby-rocksdb.db");
    check_set_get<debby::rocksdb::database>(db_path);
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021,2022 Vladislav Trifochkin
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

#if DEBBY__ROCKSDB_ENABLED
#   include "pfs/debby/backend/rocksdb/database.hpp"
#endif

namespace fs = pfs::filesystem;

template <typename Backend>
void check_set_get (pfs::filesystem::path const & db_path)
{
    using database_t = debby::keyvalue_database<Backend>;

    if (fs::exists(db_path) && fs::is_regular_file(db_path))
        fs::remove(db_path);

    auto db = database_t::make(db_path);

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

    // FIXME Cause abnormal finishing
    //db.destroy();
    //REQUIRE(!db);
    fs::remove_all(db_path);
}

#if DEBBY__ROCKSDB_ENABLED
TEST_CASE("rocksdb set/get") {
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-rocksdb.db");
    check_set_get<debby::backend::rocksdb::database>(db_path);
}
#endif

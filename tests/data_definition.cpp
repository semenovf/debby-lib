////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021-2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2024.11.21 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/debby/data_definition.hpp"
#include <pfs/filesystem.hpp>

#if DEBBY__SQLITE3_ENABLED
#   include "pfs/debby/sqlite3.hpp"
#endif
//
#if DEBBY__PSQL_ENABLED
#   include "pfs/debby/psql.hpp"
#   include "psql_support.hpp"
#endif

namespace fs = pfs::filesystem;

template <typename RelationalDatabaseType>
void check (RelationalDatabaseType & db)
{
    using data_definition_t = debby::data_definition<RelationalDatabaseType::backend_value>;

    {
        auto t = data_definition_t::create_table("table1");
        t.template add_column<std::uint32_t>("bool")
            .primary_key().unique();
        t.template add_column<bool>("bool");
        t.template add_column<std::int8_t>("int8");
        t.template add_column<std::uint16_t>("uint16");
        t.template add_column<float>("float");
        t.template add_column<std::string>("text");
        t.template add_column<debby::blob_t>("blob").nullable();

        fmt::println(t.build());
    }
}

#if DEBBY__SQLITE3_ENABLED
TEST_CASE("sqlite3") {
    using database_t = debby::relational_database<debby::backend_enum::sqlite3>;
    auto db_path = fs::temp_directory_path() / PFS__LITERAL_PATH("debby-sqlite3-dd.db");
    debby::sqlite3::wipe(db_path);

    auto db = debby::sqlite3::make(db_path);
    REQUIRE(db);

    check(db);
    debby::sqlite3::wipe(db_path);
}
#endif

#if DEBBY__PSQL_ENABLED
TEST_CASE("PostgreSQL") {
    auto conninfo = psql_conninfo();
    auto db = debby::psql::make(conninfo.cbegin(), conninfo.cend());

    if (!db) {
        MESSAGE(preconditions_notice());
    }

    REQUIRE(db);

// FIXME UNCOMMENT WHEN IMPLEMENTED
    // check(db);
}
#endif

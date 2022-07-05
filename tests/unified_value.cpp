////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `debby-lib` library.
//
// Changelog:
//      2021.12.22 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/debby/unified_value.hpp"
#include <limits>

TEST_CASE("unified_value") {
    {
        debby::unified_value x{true};
        CHECK((debby::get_if<bool>(& x) != nullptr && *debby::get_if<bool>(& x) == true));
    }

    {
        debby::unified_value x{'\x42'};

        REQUIRE(debby::get_if<bool>(& x) == nullptr);

        REQUIRE(debby::get_if<char>(& x) != nullptr);
        CHECK(*debby::get_if<char>(& x) == '\x42');

        REQUIRE(debby::get_if<int>(& x) != nullptr);
        CHECK_EQ(*debby::get_if<int>(& x), 0x42);
    }

    {
        auto sample = std::numeric_limits<std::uint64_t>::max();
        debby::unified_value x{sample};

        REQUIRE(debby::get_if<bool>(& x) == nullptr);

        REQUIRE(debby::get_if<std::uint64_t>(& x) != nullptr);
        CHECK_EQ(*debby::get_if<std::uint64_t>(& x), sample);
    }

    {
        auto sample = std::string{"hello"};
        debby::unified_value x{sample};

        REQUIRE(debby::get_if<bool>(& x) == nullptr);

        REQUIRE(debby::get_if<std::string>(& x) != nullptr);
        CHECK_EQ(*debby::get_if<std::string>(& x), sample);
    }

    {
        auto sample = debby::blob_t {'a', 'b', 'c'};
        debby::unified_value x{sample};

        REQUIRE(debby::get_if<bool>(& x) == nullptr);

        REQUIRE(debby::get_if<debby::blob_t>(& x) != nullptr);
        CHECK_EQ(*debby::get_if<debby::blob_t>(& x), sample);
    }
}

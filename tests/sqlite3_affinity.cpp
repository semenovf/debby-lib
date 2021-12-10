////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of [debby-lib](https://github.com/semenovf/debby-lib) library.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/string_view.hpp"
#include "pfs/debby/sqlite3/affinity_traits.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"

using namespace pfs::debby::sqlite3;

template <typename IntegralT>
void test_integral_type ()
{
    CHECK(std::is_same<int, typename affinity_traits<IntegralT>::storage_type>::value);
    CHECK(std::is_same<int, typename affinity_traits<IntegralT &>::storage_type>::value);
    CHECK(std::is_same<int, typename affinity_traits<IntegralT const>::storage_type>::value);
    CHECK(std::is_same<int, typename affinity_traits<IntegralT const &>::storage_type>::value);

    // Compile error (it should be )
    //CHECK(std::is_same<int, typename affinity<IntegralT const *>::storage_type>::value);

    CHECK(affinity_traits<IntegralT>::name() == "INTEGER");
    CHECK(affinity_traits<IntegralT &>::name() == "INTEGER");
    CHECK(affinity_traits<IntegralT const>::name() == "INTEGER");
    CHECK(affinity_traits<IntegralT const &>::name() == "INTEGER");
}

template <typename IntegralT>
void test_integral64_type ()
{
    CHECK(std::is_same<std::int64_t, typename affinity_traits<IntegralT>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<IntegralT &>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<IntegralT const>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<IntegralT const &>::storage_type>::value);

    CHECK(affinity_traits<IntegralT>::name() == "INTEGER");
    CHECK(affinity_traits<IntegralT &>::name() == "INTEGER");
    CHECK(affinity_traits<IntegralT const>::name() == "INTEGER");
    CHECK(affinity_traits<IntegralT const &>::name() == "INTEGER");
}

template <typename IntegralT>
void test_floating_type ()
{
    CHECK(std::is_same<double, typename affinity_traits<IntegralT>::storage_type>::value);
    CHECK(std::is_same<double, typename affinity_traits<IntegralT &>::storage_type>::value);
    CHECK(std::is_same<double, typename affinity_traits<IntegralT const>::storage_type>::value);
    CHECK(std::is_same<double, typename affinity_traits<IntegralT const &>::storage_type>::value);

    CHECK(affinity_traits<IntegralT>::name() == "REAL");
    CHECK(affinity_traits<IntegralT &>::name() == "REAL");
    CHECK(affinity_traits<IntegralT const>::name() == "REAL");
    CHECK(affinity_traits<IntegralT const &>::name() == "REAL");
}

template <typename IntegralT>
void test_string_type ()
{
    CHECK(std::is_same<std::string, typename affinity_traits<IntegralT>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<IntegralT &>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<IntegralT const>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<IntegralT const &>::storage_type>::value);

    CHECK(affinity_traits<IntegralT>::name() == "TEXT");
    CHECK(affinity_traits<IntegralT &>::name() == "TEXT");
    CHECK(affinity_traits<IntegralT const>::name() == "TEXT");
    CHECK(affinity_traits<IntegralT const &>::name() == "TEXT");
}

TEST_CASE("sqlite3 affinity") {
    test_integral_type<bool>();
    test_integral_type<std::int8_t>();
    test_integral_type<std::uint8_t>();
    test_integral_type<std::int16_t>();
    test_integral_type<std::uint16_t>();
    test_integral_type<std::int32_t>();
    test_integral_type<std::uint32_t>();

    test_integral64_type<std::int64_t>();
    test_integral64_type<std::uint64_t>();

    test_floating_type<float>();
    test_floating_type<double>();

    test_string_type<std::string>();
    test_string_type<pfs::string_view>();
}

TEST_CASE("UUID affinity") {
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::uuid_t>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::uuid_t &>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::uuid_t const>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::uuid_t const &>::storage_type>::value);

    CHECK(affinity_traits<pfs::uuid_t>::name() == "TEXT");
    CHECK(affinity_traits<pfs::uuid_t &>::name() == "TEXT");
    CHECK(affinity_traits<pfs::uuid_t const>::name() == "TEXT");
    CHECK(affinity_traits<pfs::uuid_t const &>::name() == "TEXT");
}

TEST_CASE("time_point affinity") {
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::time_point>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::time_point &>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::time_point const>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::time_point const &>::storage_type>::value);

    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::utc_time_point>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::utc_time_point &>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::utc_time_point const>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::utc_time_point const &>::storage_type>::value);

    CHECK(affinity_traits<pfs::time_point>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::time_point &>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::time_point const>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::time_point const &>::name() == "INTEGER");

    CHECK(affinity_traits<pfs::utc_time_point>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::utc_time_point &>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::utc_time_point const>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::utc_time_point const &>::name() == "INTEGER");
}

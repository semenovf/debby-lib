////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2021.12.10 Initial version.
////////////////////////////////////////////////////////////////////////////////
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "pfs/string_view.hpp"
#include "pfs/debby/backend/sqlite3/affinity_traits.hpp"
#include "pfs/debby/backend/sqlite3/sha256_traits.hpp"
#include "pfs/debby/backend/sqlite3/time_point_traits.hpp"
#include "pfs/debby/backend/sqlite3/uuid_traits.hpp"

using namespace debby::backend::sqlite3;

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

template <typename FloatT>
void test_floating_type ()
{
    CHECK(std::is_same<FloatT, typename affinity_traits<FloatT>::storage_type>::value);
    CHECK(std::is_same<FloatT, typename affinity_traits<FloatT &>::storage_type>::value);
    CHECK(std::is_same<FloatT, typename affinity_traits<FloatT const>::storage_type>::value);
    CHECK(std::is_same<FloatT, typename affinity_traits<FloatT const &>::storage_type>::value);

    CHECK(affinity_traits<FloatT>::name() == "REAL");
    CHECK(affinity_traits<FloatT &>::name() == "REAL");
    CHECK(affinity_traits<FloatT const>::name() == "REAL");
    CHECK(affinity_traits<FloatT const &>::name() == "REAL");
}

template <typename StringT>
void test_string_type ()
{
    CHECK(std::is_same<std::string, typename affinity_traits<StringT>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<StringT &>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<StringT const>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<StringT const &>::storage_type>::value);

    CHECK(affinity_traits<StringT>::name() == "TEXT");
    CHECK(affinity_traits<StringT &>::name() == "TEXT");
    CHECK(affinity_traits<StringT const>::name() == "TEXT");
    CHECK(affinity_traits<StringT const &>::name() == "TEXT");
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

    CHECK(std::is_same<std::string, typename affinity_traits<char const *>::storage_type>::value);
    CHECK(affinity_traits<char const *>::name() == "TEXT");
}

TEST_CASE("SHA256 affinity") {
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::crypto::sha256_digest>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::crypto::sha256_digest &>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::crypto::sha256_digest const>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::crypto::sha256_digest const &>::storage_type>::value);

    CHECK(affinity_traits<pfs::crypto::sha256_digest>::name() == "TEXT");
    CHECK(affinity_traits<pfs::crypto::sha256_digest &>::name() == "TEXT");
    CHECK(affinity_traits<pfs::crypto::sha256_digest const>::name() == "TEXT");
    CHECK(affinity_traits<pfs::crypto::sha256_digest const &>::name() == "TEXT");
}

TEST_CASE("UUID affinity") {
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::universal_id>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::universal_id &>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::universal_id const>::storage_type>::value);
    CHECK(std::is_same<std::string, typename affinity_traits<pfs::universal_id const &>::storage_type>::value);

    CHECK(affinity_traits<pfs::universal_id>::name() == "TEXT");
    CHECK(affinity_traits<pfs::universal_id &>::name() == "TEXT");
    CHECK(affinity_traits<pfs::universal_id const>::name() == "TEXT");
    CHECK(affinity_traits<pfs::universal_id const &>::name() == "TEXT");
}

TEST_CASE("time_point affinity") {
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::local_time_point>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::local_time_point &>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::local_time_point const>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::local_time_point const &>::storage_type>::value);

    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::utc_time_point>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::utc_time_point &>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::utc_time_point const>::storage_type>::value);
    CHECK(std::is_same<std::int64_t, typename affinity_traits<pfs::utc_time_point const &>::storage_type>::value);

    CHECK(affinity_traits<pfs::local_time_point>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::local_time_point &>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::local_time_point const>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::local_time_point const &>::name() == "INTEGER");

    CHECK(affinity_traits<pfs::utc_time_point>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::utc_time_point &>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::utc_time_point const>::name() == "INTEGER");
    CHECK(affinity_traits<pfs::utc_time_point const &>::name() == "INTEGER");
}

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
#include "pfs/debby/sqlite3/cast_traits.hpp"
#include "pfs/debby/sqlite3/time_point_traits.hpp"
#include "pfs/debby/sqlite3/uuid_traits.hpp"
#include <cstdlib>

using namespace pfs::debby::sqlite3;

template <typename IntegralT>
void test_signed_integral_type ()
{
    CHECK(cast_traits<IntegralT>::to_storage(0) == 0);
    CHECK(cast_traits<IntegralT>::to_storage(42) == 42);
    CHECK(cast_traits<IntegralT>::to_storage(-1) == -1);
    CHECK(cast_traits<IntegralT>::to_storage(std::numeric_limits<IntegralT>::max()) == std::numeric_limits<IntegralT>::max());
    CHECK(cast_traits<IntegralT>::to_storage(std::numeric_limits<IntegralT>::min()) == std::numeric_limits<IntegralT>::min());
    CHECK(*cast_traits<IntegralT>::to_native(0) == 0);
    CHECK(*cast_traits<IntegralT>::to_native(42) == 42);
    CHECK(*cast_traits<IntegralT>::to_native(-1) == -1);
    CHECK(cast_traits<IntegralT>::to_native(std::numeric_limits<IntegralT>::max()) == std::numeric_limits<IntegralT>::max());
    CHECK(cast_traits<IntegralT>::to_native(std::numeric_limits<IntegralT>::min()) == std::numeric_limits<IntegralT>::min());
}

template <typename IntegralT>
void test_unsigned_integral_type ()
{
    CHECK(cast_traits<IntegralT>::to_storage(0) == 0);
    CHECK(cast_traits<IntegralT>::to_storage(42) == 42);
    CHECK(cast_traits<IntegralT>::to_storage(std::numeric_limits<IntegralT>::max()) == std::numeric_limits<IntegralT>::max());
    CHECK(*cast_traits<IntegralT>::to_native(0) == 0);
    CHECK(*cast_traits<IntegralT>::to_native(42) == 42);
    CHECK(*cast_traits<IntegralT>::to_native(std::numeric_limits<IntegralT>::max()) == std::numeric_limits<IntegralT>::max());
}

template <typename FloatT>
void test_floating_type ()
{
    CHECK(cast_traits<FloatT>::to_storage(0) == 0);
    CHECK(std::abs(cast_traits<FloatT>::to_storage(42.42) - 42.42) < 0.01);
    CHECK(std::abs(cast_traits<FloatT>::to_storage(-1.1) - (-1.1)) < 0.01);
    CHECK(cast_traits<FloatT>::to_storage(std::numeric_limits<FloatT>::max()) == std::numeric_limits<FloatT>::max());
    CHECK(cast_traits<FloatT>::to_storage(std::numeric_limits<FloatT>::min()) == std::numeric_limits<FloatT>::min());
    CHECK(*cast_traits<FloatT>::to_native(0) == 0);
    CHECK(std::abs(*cast_traits<FloatT>::to_native(42.42) - 42.42) < 0.01);
    CHECK(std::abs(*cast_traits<FloatT>::to_native(-1.1) - (-1.1)) < 0.01);
    CHECK(*cast_traits<FloatT>::to_native(std::numeric_limits<FloatT>::max()) == std::numeric_limits<FloatT>::max());
    CHECK(*cast_traits<FloatT>::to_native(std::numeric_limits<FloatT>::min()) == std::numeric_limits<FloatT>::min());
}

TEST_CASE("cast traits") {
    CHECK(cast_traits<bool>::to_storage(true) != 0);
    CHECK(cast_traits<bool>::to_storage(false) == 0);
    CHECK(*cast_traits<bool>::to_native(1) == true);
    CHECK(*cast_traits<bool>::to_native(42) == true);
    CHECK(*cast_traits<bool>::to_native(0) == false);

    CHECK(cast_traits<char>::to_storage('\x0') == 0);
    CHECK(cast_traits<char>::to_storage('\x20') == 0x20);
    CHECK(cast_traits<char>::to_storage(char{-1}) == -1);
    CHECK(cast_traits<char>::to_storage(std::numeric_limits<char>::max()) == std::numeric_limits<char>::max());
    CHECK(cast_traits<char>::to_storage(std::numeric_limits<char>::min()) == std::numeric_limits<char>::min());
    CHECK(*cast_traits<char>::to_native(0) == '\x0');
    CHECK(*cast_traits<char>::to_native(0x20) == ' ');
    CHECK(*cast_traits<char>::to_native(-1) == char{-1});
    CHECK(*cast_traits<char>::to_native(std::numeric_limits<char>::max()) == std::numeric_limits<char>::max());
    CHECK(*cast_traits<char>::to_native(std::numeric_limits<char>::min()) == std::numeric_limits<char>::min());

    test_signed_integral_type<signed char>();
    test_signed_integral_type<short>();
    test_signed_integral_type<int>();
    test_signed_integral_type<long>();
    test_signed_integral_type<long long>();

    test_unsigned_integral_type<unsigned char>();
    test_unsigned_integral_type<unsigned short>();
    test_unsigned_integral_type<unsigned int>();
    test_unsigned_integral_type<unsigned long>();
    test_unsigned_integral_type<unsigned long long>();

    test_floating_type<float>();
    test_floating_type<double>();

    CHECK(cast_traits<std::string>::to_storage("") == std::string{});
    CHECK(cast_traits<std::string>::to_storage("hello") == std::string{"hello"});
    CHECK(cast_traits<pfs::string_view>::to_storage("") == std::string{});
    CHECK(cast_traits<pfs::string_view>::to_storage("hello") == std::string{"hello"});

    CHECK(*cast_traits<std::string>::to_native("") == std::string{});
    CHECK(*cast_traits<std::string>::to_native("hello") == std::string{"hello"});

    // Forbidden conversion
    //CHECK(cast_traits<pfs::string_view>::to_native("") == pfs::string_view{});
    //CHECK(cast_traits<pfs::string_view>::to_native("hello") == pfs::string_view{"hello"});
}

TEST_CASE("UUID cast traits") {
    CHECK(cast_traits<pfs::uuid_t>::to_storage("01D78XYFJ1PRM1WPBCBT3VHMNV"_uuid) == std::string{"01D78XYFJ1PRM1WPBCBT3VHMNV"});
    CHECK(*cast_traits<pfs::uuid_t>::to_native("01D78XYFJ1PRM1WPBCBT3VHMNV") == "01D78XYFJ1PRM1WPBCBT3VHMNV"_uuid);
    CHECK_FALSE(cast_traits<pfs::uuid_t>::to_native("").has_value());
    CHECK_FALSE(cast_traits<pfs::uuid_t>::to_native("01D78XYF").has_value());
}

TEST_CASE("time_point cast traits") {
    using pfs::time_point;
    using pfs::utc_time_point;

    auto tp = pfs::from_iso8601("2021-11-22T11:33:42.999+0500");

    REQUIRE(tp.has_value());
    CHECK(cast_traits<time_point>::to_storage(tp->value) == pfs::to_millis(tp->value).count());
    CHECK(cast_traits<utc_time_point>::to_storage(*tp) == pfs::to_millis(*tp).count());

    CHECK(*cast_traits<time_point>::to_native(pfs::to_millis(tp->value).count()) == tp->value);
    CHECK(cast_traits<utc_time_point>::to_native(pfs::to_millis(tp->value).count())->value == tp->value);
}

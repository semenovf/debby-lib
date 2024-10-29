////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2023 Vladislav Trifochkin
//
// This file is part of `debby-lib`.
//
// Changelog:
//      2023.11.26 Initial version (based on sqlite3_cast_traits.cpp).
////////////////////////////////////////////////////////////////////////////////

template <typename IntegralT>
void test_signed_integral_type ()
{
    CHECK(cast_traits<IntegralT>::to_storage(0) == 0);
    CHECK(cast_traits<IntegralT>::to_storage(42) == 42);
    CHECK(cast_traits<IntegralT>::to_storage(-1) == -1);
    CHECK(cast_traits<IntegralT>::to_storage(std::numeric_limits<IntegralT>::max()) == std::numeric_limits<IntegralT>::max());
    CHECK(cast_traits<IntegralT>::to_storage(std::numeric_limits<IntegralT>::min()) == std::numeric_limits<IntegralT>::min());
    CHECK(cast_traits<IntegralT>::to_native(0) == 0);
    CHECK(cast_traits<IntegralT>::to_native(42) == 42);
    CHECK(cast_traits<IntegralT>::to_native(-1) == -1);
    CHECK(cast_traits<IntegralT>::to_native(std::numeric_limits<IntegralT>::max()) == std::numeric_limits<IntegralT>::max());
    CHECK(cast_traits<IntegralT>::to_native(std::numeric_limits<IntegralT>::min()) == std::numeric_limits<IntegralT>::min());
}

template <typename IntegralT>
void test_unsigned_integral_type ()
{
    CHECK_EQ(cast_traits<IntegralT>::to_storage(0), 0);
    CHECK_EQ(cast_traits<IntegralT>::to_storage(42), 42);
    CHECK_EQ(static_cast<IntegralT>(cast_traits<IntegralT>::to_storage(std::numeric_limits<IntegralT>::max())), std::numeric_limits<IntegralT>::max());
    CHECK_EQ(cast_traits<IntegralT>::to_native(0), 0);
    CHECK_EQ(cast_traits<IntegralT>::to_native(42), 42);
    CHECK_EQ(cast_traits<IntegralT>::to_native(std::numeric_limits<IntegralT>::max()), std::numeric_limits<IntegralT>::max());
}

template <typename FloatT>
void test_floating_type ()
{
    CHECK(cast_traits<FloatT>::to_storage(0) == 0);
    CHECK(std::abs(cast_traits<FloatT>::to_storage(42.42) - 42.42) < 0.01);
    CHECK(std::abs(cast_traits<FloatT>::to_storage(-1.1) - (-1.1)) < 0.01);
    CHECK(cast_traits<FloatT>::to_storage(std::numeric_limits<FloatT>::max()) == std::numeric_limits<FloatT>::max());
    CHECK(cast_traits<FloatT>::to_storage(std::numeric_limits<FloatT>::min()) == std::numeric_limits<FloatT>::min());
    CHECK(cast_traits<FloatT>::to_native(0) == 0);
    CHECK(std::abs(cast_traits<FloatT>::to_native(42.42) - 42.42) < 0.01);
    CHECK(std::abs(cast_traits<FloatT>::to_native(-1.1) - (-1.1)) < 0.01);
    CHECK(cast_traits<FloatT>::to_native(std::numeric_limits<FloatT>::max()) == std::numeric_limits<FloatT>::max());
    CHECK(cast_traits<FloatT>::to_native(std::numeric_limits<FloatT>::min()) == std::numeric_limits<FloatT>::min());
}

TEST_CASE("cast traits") {
    CHECK(cast_traits<bool>::to_storage(true) != 0);
    CHECK(cast_traits<bool>::to_storage(false) == 0);
    CHECK(cast_traits<bool>::to_native(1) == true);
    CHECK(cast_traits<bool>::to_native(42) == true);
    CHECK(cast_traits<bool>::to_native(0) == false);

    CHECK(cast_traits<char>::to_storage('\x0') == 0);
    CHECK(cast_traits<char>::to_storage('\x20') == 0x20);
    CHECK(cast_traits<char>::to_storage(char{-1}) == -1);
    CHECK(cast_traits<char>::to_storage(std::numeric_limits<char>::max()) == std::numeric_limits<char>::max());
    CHECK(cast_traits<char>::to_storage(std::numeric_limits<char>::min()) == std::numeric_limits<char>::min());
    CHECK(cast_traits<char>::to_native(0) == '\x0');
    CHECK(cast_traits<char>::to_native(0x20) == ' ');
    CHECK(cast_traits<char>::to_native(-1) == char{-1});
    CHECK(cast_traits<char>::to_native(std::numeric_limits<char>::max()) == std::numeric_limits<char>::max());
    CHECK(cast_traits<char>::to_native(std::numeric_limits<char>::min()) == std::numeric_limits<char>::min());

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

    CHECK(cast_traits<std::string>::to_native("") == std::string{});
    CHECK(cast_traits<std::string>::to_native("hello") == std::string{"hello"});

    // Forbidden conversion
    //CHECK(cast_traits<pfs::string_view>::to_native("") == pfs::string_view{});
    //CHECK(cast_traits<pfs::string_view>::to_native("hello") == pfs::string_view{"hello"});
}

TEST_CASE("UUID cast traits") {
    CHECK(cast_traits<pfs::universal_id>::to_storage("01D78XYFJ1PRM1WPBCBT3VHMNV"_uuid) == std::string{"01D78XYFJ1PRM1WPBCBT3VHMNV"});
    CHECK(cast_traits<pfs::universal_id>::to_native("01D78XYFJ1PRM1WPBCBT3VHMNV") == "01D78XYFJ1PRM1WPBCBT3VHMNV"_uuid);
    CHECK(cast_traits<pfs::universal_id>::to_native("") == pfs::universal_id{});
    CHECK(cast_traits<pfs::universal_id>::to_native("01D78XYF") == pfs::universal_id{});
}

TEST_CASE("time_point cast traits") {
    using pfs::local_time_point;
    using pfs::utc_time_point;

    auto u = utc_time_point::from_iso8601("2021-11-22T11:33:42.999+0500");
    CHECK(cast_traits<utc_time_point>::to_storage(u) == u.to_millis().count());
    CHECK(cast_traits<utc_time_point>::to_native(u.to_millis().count()) == u);

    auto l = pfs::local_time_point_cast(u);
    CHECK(cast_traits<local_time_point>::to_storage(l) == l.to_millis().count());
    CHECK(cast_traits<local_time_point>::to_native(l.to_millis().count()) == l);
}

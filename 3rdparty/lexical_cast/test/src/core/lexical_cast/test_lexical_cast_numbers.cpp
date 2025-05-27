// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023, 2024 by Mark Melton
//

#include "core/lexical_cast/error.h"
#include "core/lexical_cast/floating.h"
#include "core/lexical_cast/integral.h"
#include "core/mp/traits/extrema.h"
#include "coro/stream/stream.h"
#include <gtest/gtest.h>

using namespace core;

template <typename T>
struct number_test {
    static void apply() {
        auto actual_min_value = mp::extrema<T>::min();
        auto min_repr =
            fmt::format(fmt::runtime(mp::extrema<T>::fmt_spec()), actual_min_value);
        auto min_value = lexical_cast<T>(min_repr);
        EXPECT_EQ(min_value, actual_min_value);

        auto actual_max_value = mp::extrema<T>::max();
        auto max_repr =
            fmt::format(fmt::runtime(mp::extrema<T>::fmt_spec()), actual_max_value);
        auto max_value = lexical_cast<T>(max_repr);
        EXPECT_EQ(max_value, actual_max_value);

        EXPECT_THROW(lexical_cast<T>("abc"), lexical_cast_error);

        auto generator = coro::sampler<T>(actual_min_value, actual_max_value);
        for (auto value : std::move(generator) | coro::take(5)) {
            std::string s = fmt::format(fmt::runtime(mp::extrema<T>::fmt_spec()), value);
            auto n = lexical_cast<T>(s);
            EXPECT_EQ(value, n);
            EXPECT_EQ(lexical_cast<T>(lexical_to_string(n)), n);
        }
    }
};

template <class T>
void hex_test() {
    auto rng = std::random_device();
    for (auto i = 0; i < 256; ++i) {
        T n{(T)rng()};
        auto s = fmt::format("0x{:x}", n);
        auto m = lexical_cast<T>(s);
        EXPECT_EQ(m, n);
        EXPECT_EQ(lexical_cast<T>(lexical_to_string(m)), m);
    }
}

TEST(LexicalCast, SignedChar) {
    number_test<signed char>::apply();
    hex_test<signed char>();
}

TEST(LexicalCast, SignedShort) {
    number_test<signed short>::apply();
    hex_test<signed short>();
}

TEST(LexicalCast, SignedInt) {
    number_test<signed int>::apply();
    hex_test<signed int>();
}

using ZeroTestCase = std::tuple<std::string, int>;
using ZeroTestCases = std::vector<ZeroTestCase>;
static ZeroTestCases signed_test_cases = {
    {"0", 0}, {"0000", 0}, {"-0", 0}, {"-0000", 0}, {"+0", 0}, {"+0000", 0}};

TEST(LexicalCast, SignedIntZero) {
    for (const auto& [str, expected] : signed_test_cases) {
	auto n = lexical_cast<int>(str);
	EXPECT_EQ(n, expected);
    }
}

TEST(LexicalCast, SignedLong) {
    number_test<signed long>::apply();
    hex_test<signed long>();
}

TEST(LexicalCast, SignedLongLong) {
    number_test<signed long long>::apply();
    hex_test<signed long long>();
}

TEST(LexicalCast, UnsignedChar) {
    number_test<unsigned char>::apply();
    hex_test<unsigned char>();
}

TEST(LexicalCast, UnsignedShort) {
    number_test<unsigned short>::apply();
    hex_test<unsigned short>();
}

TEST(LexicalCast, UnsignedInt) {
    number_test<unsigned int>::apply();
    hex_test<unsigned int>();
}

TEST(LexicalCast, UnsignedLong) {
    number_test<unsigned long>::apply();
    hex_test<unsigned long>();
}

TEST(LexicalCast, UnsignedLongLong) {
    number_test<unsigned long long>::apply();
    hex_test<unsigned long long>();
}

TEST(LexicalCast, Int8) {
    number_test<std::int8_t>::apply();
    hex_test<std::int8_t>();
}

TEST(LexicalCast, Int16) {
    number_test<std::int16_t>::apply();
    hex_test<std::int16_t>();
}

TEST(LexicalCast, Int32) {
    number_test<std::int32_t>::apply();
    hex_test<std::int32_t>();
}

TEST(LexicalCast, Int64) {
    number_test<std::int64_t>::apply();
    hex_test<std::int64_t>();
}

TEST(LexicalCast, Uint8) {
    number_test<std::uint8_t>::apply();
    hex_test<std::uint8_t>();
}

TEST(LexicalCast, Uint16) {
    number_test<std::uint16_t>::apply();
    hex_test<std::uint16_t>();
}

TEST(LexicalCast, Uint32) {
    number_test<std::uint32_t>::apply();
    hex_test<std::uint32_t>();
}

TEST(LexicalCast, Uint64) {
    number_test<std::uint64_t>::apply();
    hex_test<std::uint64_t>();
}

TEST(LexicalCast, Real32) { number_test<float>::apply(); }

TEST(LexicalCast, Real64) { number_test<double>::apply(); }

TEST(LexicalCast, Real128) { number_test<long double>::apply(); }

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

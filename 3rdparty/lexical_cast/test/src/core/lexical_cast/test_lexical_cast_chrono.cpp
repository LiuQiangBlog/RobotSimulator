// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023, 2024 by Mark Melton
//

#include <gtest/gtest.h>

#include "core/lexical_cast/chrono.h"
#include "core/lexical_cast/error.h"
#include "testing.h"

inline constexpr auto NumberSamples = 16;

CHECK_LEXICAL();

TEST(LexicalCast, ChronoUniversal) {
    UNIVERSAL_TEST(test);
    fold_seq<std::chrono::seconds,
             std::chrono::minutes,
             std::chrono::hours,
	     std::chrono::day,
             std::chrono::days,
             std::chrono::weeks,
	     std::chrono::month,
             std::chrono::months,
             std::chrono::year,
             std::chrono::years>{}(test, NumberSamples);
}

TEST(LexicalCast, ChronoToString) {
    EXPECT_EQ(lexical_to_string(std::chrono::seconds{-5}), "-5");
    EXPECT_EQ(lexical_to_string(std::chrono::hours{+5}), "5");
}

TEST(LexicalCast, ChronoFromStringThrow) {
    EXPECT_THROW(lexical_cast<std::chrono::seconds>("a"), lexical_cast_error);
    EXPECT_THROW(lexical_cast<std::chrono::minutes>("2a"), lexical_cast_error);
    EXPECT_THROW(lexical_cast<std::chrono::hours>("a2"), lexical_cast_error);
}

int main(int argc, char *argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

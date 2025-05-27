// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <gtest/gtest.h>
#include "core/lexical_cast/bool.h"
#include "core/lexical_cast/error.h"
#include "testing.h"

inline constexpr auto NumberSamples = 16;

CHECK_LEXICAL();

TEST(LexicalCast, BoolGenerative)
{
    UNIVERSAL_TEST(test);
    fold_seq<bool>{}(test, NumberSamples);
}

TEST(LexicalCast, BoolConvert)
{
    for (auto str : {"0", "f", "F", "false", " 0 ", " f ", " F ", "  false  "})
	check_lexical(str, false);

    for (auto str : {"1", "t", "T", "true", " 1 ", " t ", " T ", " true "})
	check_lexical(str, true);
}

TEST(LexicalCast, BoolToString)
{
    EXPECT_EQ(lexical_to_string(true), "t");
    EXPECT_EQ(lexical_to_string(false), "f");
}

TEST(LexicalCast, BoolFromStringThrow)
{
    EXPECT_THROW(lexical_cast<bool>(""), lexical_cast_error);
    EXPECT_THROW(lexical_cast<bool>("2"), lexical_cast_error);
    EXPECT_THROW(lexical_cast<bool>("a"), lexical_cast_error);
    EXPECT_THROW(lexical_cast<bool>("tt"), lexical_cast_error);
    EXPECT_THROW(lexical_cast<bool>("falsef"), lexical_cast_error);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

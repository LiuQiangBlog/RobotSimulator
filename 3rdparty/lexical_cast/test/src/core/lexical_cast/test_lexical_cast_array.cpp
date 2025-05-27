// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <gtest/gtest.h>
#include "core/lexical_cast/array.h"
#include "core/lexical_cast/vector.h"
#include "core/lexical_cast/pair.h"
#include "core/lexical_cast/string.h"
#include "testing.h"

using namespace std::string_literals;

inline constexpr auto NumberSamples = 64;

using test_types = std::tuple<
    std::array<unsigned int, 4>,
    std::array<std::string, 3>,
    std::array<std::pair<std::string,int>, 5>
    >;

CHECK_LEXICAL();

TEST(LexicalCast, ArrayGenerative)
{
    UNIVERSAL_TEST(test);
    fold_seq_list<test_types>{}(test, NumberSamples);
}

TEST(LexicalCast, ArrayConvert)
{
    std::array<int, 3> a0{ 123, 456, 789 };
    std::array<std::vector<int>, 2> a1{std::vector{12, 34}, {56}};
    std::array<std::pair<int,std::string>, 2> a2{ std::pair{123, "abc"s}, std::pair{456, "def"s} };
    check_lexical("123,456, 789", a0);
    check_lexical("{123,456,789 }", a0);
    check_lexical("{12,34} ,56", a1);
    check_lexical(" { 12, 34},{56}", a1);
    check_lexical("{123, abc } , { 456,def }", a2);
    check_lexical("[{123,abc},{456,def}]", a2);

    EXPECT_EQ(lexical_to_string(a0), "[123,456,789]");
    EXPECT_EQ(lexical_to_string(a1), "[[12,34],[56]]");
    EXPECT_EQ(lexical_to_string(a2), "[(123,\"abc\"),(456,\"def\")]");
}

TEST(LexicalCast, ArrayThrow)
{
    EXPECT_THROW((lexical_cast<std::array<int, 2>>(" 123 , abc ")), lexical_cast_error);
    EXPECT_THROW((lexical_cast<std::array<int, 2>>(" 123 ")), lexical_cast_error);
    EXPECT_THROW((lexical_cast<std::array<int, 2>>("123,456,789")), lexical_cast_error);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

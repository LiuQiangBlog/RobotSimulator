// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <gtest/gtest.h>
#include "core/lexical_cast/pair.h"
#include "core/lexical_cast/string.h"
#include "testing.h"

inline constexpr auto NumberSamples = 64;

using test_types = std::tuple<std::pair<int, std::string>>;

CHECK_LEXICAL();

TEST(LexicalCast, PairGenerative)
{
    UNIVERSAL_TEST(test);
    fold_seq_list<test_types>{}(test, NumberSamples);
}

TEST(LexicalCast, PairConvert)
{
    auto p0 = std::make_pair(123, std::string{"abc"});
    auto p1 = std::make_pair(std::make_pair(123, std::string{"abc"}), 1.0);
    
    check_lexical(" 123 , abc ", p0);
    check_lexical(" ( 123 , abc ) ", p0);
    check_lexical("(123,abc),1.0", p1);
    check_lexical("((123,abc),1.0)", p1);
}

TEST(LexicalCast, ToString)
{
    auto p0 = std::make_pair(123, std::string{"abc"});
    auto p1 = std::make_pair(std::make_pair(123, std::string{"abc"}), 1.0);
    
    EXPECT_EQ(lexical_to_string(p0), R"((123,"abc"))");
    EXPECT_EQ(lexical_to_string(p1), R"(((123,"abc"),1.000000))");
}

TEST(LexicalCast, PairThrow)
{
    EXPECT_THROW((lexical_cast<std::pair<int,std::string>>("123")), lexical_cast_error);
    EXPECT_THROW((lexical_cast<std::pair<int,std::string>>("12x:abc")), lexical_cast_error);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

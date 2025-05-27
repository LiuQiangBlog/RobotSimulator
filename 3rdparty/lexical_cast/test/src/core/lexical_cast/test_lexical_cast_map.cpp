// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <gtest/gtest.h>
#include "core/lexical_cast/map.h"
#include "core/lexical_cast/pair.h"
#include "core/lexical_cast/string.h"
#include "testing.h"

using namespace std::string_literals;

inline constexpr auto NumberSamples = 64;

using test_types = std::tuple<
    std::map<std::string,unsigned int>,
    std::map<int,std::string>
    >;

CHECK_LEXICAL();

TEST(LexicalCast, MapGenerative)
{
    UNIVERSAL_TEST(test);
    fold_seq_list<test_types>{}(test, NumberSamples);
}

TEST(LexicalCast, MapConvert)
{
    auto m0 = std::map<std::string,int>{{"abc",123}, {"def",456}, {"ghi",789}};
    check_lexical("{(abc,123),(def,456),(ghi,789)}", m0);
    check_lexical(R"({("abc",123),("def",456),("ghi",789)})", m0);
    check_lexical(" { ( abc , 123 ) , ( def , 456 ) , ( ghi , 789 ) } ", m0);

    EXPECT_EQ(lexical_to_string(m0), R"({("abc",123),("def",456),("ghi",789)})");
}

TEST(LexicalCast, MapThrow)
{
    EXPECT_THROW((lexical_cast<std::map<std::string,int>>("123,abc")), lexical_cast_error);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

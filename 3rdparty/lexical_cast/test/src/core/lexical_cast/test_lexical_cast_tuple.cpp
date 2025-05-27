// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <gtest/gtest.h>
#include "core/lexical_cast/tuple.h"
#include "core/lexical_cast/string.h"
#include "testing.h"

using namespace std::string_literals;

inline constexpr auto NumberSamples = 64;

CHECK_LEXICAL();

using test_types = std::tuple<
    std::tuple<unsigned int,std::string,double>,
    std::tuple<std::string>,
    std::tuple<std::string, bool>
    >;

TEST(LexicalCast, TupleGenerative)
{
    UNIVERSAL_TEST(test);
    fold_seq_list<test_types>{}(test, NumberSamples);
}

TEST(LexicalCast, TupleConvert)
{
    auto tup0 = std::make_tuple(123, "abc"s);
    check_lexical("123,abc", tup0);
    check_lexical("(123,abc)", tup0);
    check_lexical(" 123 , abc ", tup0);
    check_lexical(" ( 123 , abc ) ", tup0);
}

TEST(LexicalCast, TupleThrow)
{
    EXPECT_THROW((lexical_cast<std::tuple<int, int>>("123,abc")), lexical_cast_error);
    EXPECT_THROW((lexical_cast<std::tuple<int, int>>("123")), lexical_cast_error);
    EXPECT_THROW((lexical_cast<std::tuple<int, int>>("123,456,789")), lexical_cast_error);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

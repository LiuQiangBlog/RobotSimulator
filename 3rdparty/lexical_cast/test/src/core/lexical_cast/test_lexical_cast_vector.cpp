// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <gtest/gtest.h>
#include "core/lexical_cast/vector.h"
#include "core/lexical_cast/pair.h"
#include "core/lexical_cast/string.h"
#include "testing.h"

using namespace std::string_literals;

inline constexpr auto NumberSamples = 64;

using test_types = std::tuple<
    std::vector<unsigned int>,
    std::vector<std::string>,
    std::vector<std::pair<int,std::string>>
    >;

CHECK_LEXICAL();

TEST(LexicalCast, VectorGenerative)
{
    UNIVERSAL_TEST(test);
    fold_seq_list<test_types>{}(test, NumberSamples);
}

TEST(LexicalCast, VectorConvert)
{
    auto v0 = std::vector<int>{123, 456, 789};
    auto v1 = std::vector{std::vector<int>{12, 34}, std::vector<int>{56, 78}};
    auto v2 = std::vector{std::pair{123, "abc"s}, std::pair{456, "def"s}};
    check_lexical("123,456,789", v0);
    check_lexical("[123,456,789]", v0);
    check_lexical("(12,34),(56,78)", v1);
    check_lexical("[(12,34),(56,78)]", v1);
    check_lexical("(123,abc),(456,def)", v2);
    check_lexical("[(123,abc),(456,def)]", v2);

    EXPECT_EQ(lexical_to_string(v0), "[123,456,789]");
    EXPECT_EQ(lexical_to_string(v1), "[[12,34],[56,78]]");
    EXPECT_EQ(lexical_to_string(v2), "[(123,\"abc\"),(456,\"def\")]");
}

TEST(LexicalCast, VectorThrow)
{
    EXPECT_THROW((lexical_cast<std::vector<int>>("123,abc")), lexical_cast_error);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

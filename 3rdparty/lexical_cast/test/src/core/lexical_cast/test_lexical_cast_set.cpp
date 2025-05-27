// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <string>
#include "testing.h"
#include "core/lexical_cast/set.h"
#include "core/lexical_cast/pair.h"
#include "core/lexical_cast/string.h"

inline constexpr auto NumberSamples = 64;

using test_types = std::tuple<
    std::set<unsigned int>,
    std::set<int>,
    std::set<std::string>
    >;

CHECK_LEXICAL();

TEST(LexicalCast, SetGenerative)
{
    UNIVERSAL_TEST(test);
    fold_seq_list<test_types>{}(test, NumberSamples);
}

TEST(LexicalCast, SetConvert)
{
    std::set<int> s0{123, 456, 789};
    std::set<std::string> s1{""};
    std::set<std::set<int>> s2{{12,34},{56,78}};
    std::set<std::pair<int,std::string>> s3{{123,"abc"},{456,"def"}};
    check_lexical("123,456,789", s0);
    check_lexical("[123,456,789]", s0);
    check_lexical("[\"\"]", s1);
    check_lexical("(12,34),(56,78)", s2);
    check_lexical("[(12,34),(56,78)]", s2);
    check_lexical("(123,abc),(456,def)", s3);
    check_lexical("[(123,abc),(456,def)]", s3);

    EXPECT_EQ(lexical_to_string(s0), "[123,456,789]");
    EXPECT_EQ(lexical_to_string(s1), "[\"\"]");
    EXPECT_EQ(lexical_to_string(s2), "[[12,34],[56,78]]");
    EXPECT_EQ(lexical_to_string(s3), "[(123,\"abc\"),(456,\"def\")]");
}


TEST(LexicalCast, SetThrow)
{
    EXPECT_THROW((lexical_cast<std::set<int>>("123,abc")), lexical_cast_error);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

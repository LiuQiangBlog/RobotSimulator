// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <gtest/gtest.h>
#include "core/lexical_cast/string.h"
#include "testing.h"

using namespace std::string_literals;

const auto NumberSamples = 64;

CHECK_LEXICAL();

TEST(LexicalCast, StringGenerative)
{
    for (auto input : sampler<std::string>() | take(NumberSamples)) {
	auto str = lexical_cast<std::string>(lexical_to_string(input));
	EXPECT_EQ(str, input);
    }
}

TEST(LexicalCast, StringConvert)
{
    check_lexical("abc", "abc"s);
    check_lexical("\"abc\"", "abc"s);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

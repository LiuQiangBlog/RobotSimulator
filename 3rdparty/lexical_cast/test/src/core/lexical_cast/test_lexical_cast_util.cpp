// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#include <string>
#include <gtest/gtest.h>
#include "core/lexical_cast/util.h"

using namespace core::lexical_cast_detail;
using namespace std::string_literals;

TEST(LexicalCast, UnwrapPlain)
{
    for (auto str : {"12,34,56"s, " 12,34,56 "s}) {
	auto [b, e] = unwrap(str.begin(), str.end());
	EXPECT_EQ(std::string_view(b, e), "12,34,56"s);
    }
}

TEST(LexicalCast, UnwrapDelimitted)
{
    for (auto str : {"{12,34,56}"s, " {12,34,56} "s, "[12,34,56] "s}) {
	auto [b, e] = unwrap(str.begin(), str.end());
	EXPECT_EQ(std::string_view(b, e), "12,34,56"s);
    }
}

TEST(LexicalCast, UnwrapNested)
{
    for (auto str : {"{12,34},{56,78}"s, "{{12,34},{56,78}}"s}) {
	auto [b, e] = unwrap(str.begin(), str.end());
	EXPECT_EQ(std::string_view(b, e), "{12,34},{56,78}"s);
    }
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023, 2024 by Mark Melton
//

#include <gtest/gtest.h>
#include "core/lexical_cast/optional.h"
#include "core/lexical_cast/string.h"
#include "testing.h"

inline constexpr auto NumberSamples = 64;

using test_types = std::tuple<std::optional<int>>;

CHECK_LEXICAL();

// Need to implement optional sampler to use the generative test
// 
// TEST(LexicalCast, PairGenerative)
// {
//     UNIVERSAL_TEST(test);
//     fold_seq_list<test_types>{}(test, NumberSamples);
// }

TEST(LexicalCast, OptionalConvert)
{
    std::optional<int> v0{42}, v1{};
    check_lexical("42", v0);
    check_lexical("", v1);
}

TEST(LexicalCast, ToString)
{
    std::optional<int> v0{42}, v1{};
    EXPECT_EQ(lexical_to_string(v0), R"(42)");
    EXPECT_EQ(lexical_to_string(v1), R"()");
}

TEST(LexicalCast, PairThrow)
{
    EXPECT_THROW((lexical_cast<std::optional<int>>("abc")), lexical_cast_error);
}

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

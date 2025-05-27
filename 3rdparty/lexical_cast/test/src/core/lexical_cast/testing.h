// Copyright 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <gtest/gtest.h>
#include "coro/stream/stream.h"

using namespace core;
using namespace coro;
using std::cout, std::endl;

#define CHECK_LEXICAL()							\
    template<class T>							\
    void check_lexical(std::string_view input, const T& value) {	\
	auto s = lexical_cast<T>(input);				\
	EXPECT_EQ(s, value);						\
	auto r = lexical_cast<T>(lexical_to_string(s));			\
	EXPECT_EQ(r, s);						\
    }

#define UNIVERSAL_TEST(NAME)						\
    auto NAME = []<class T>(size_t n) {					\
	for (auto s : sampler<T>() | take(n))				\
	    EXPECT_EQ(lexical_cast<T>(lexical_to_string(s)), s);	\
    };

template<template<class...> class Op, class T>
struct dapply;

template<template<class...> class Op, template<class...> class T, class... Ts>
struct dapply<Op, T<Ts...>> {
    template<class... Args>
    auto operator()(Args&&... args) const {
	return Op<Ts...>{}(std::forward<Args>(args)...);
    }
};

template<class... Ts>
struct fold_seq {
    template<class F, class... Args>
    auto operator()(F&& func, Args&&... args) const {
	return (func.template operator()<Ts>(std::forward<Args>(args)...), ...);
    }
};

template<class T>
using fold_seq_list = dapply<fold_seq, T>;

// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <string>
#include "impl.h"
#include "error.h"
#include "util.h"

namespace core::lexical_cast_detail {

template<>
struct lexical_cast_impl<std::string> {
    std::string convert(std::string_view input) const {
	auto [b, e] = unwrap(input.begin(), input.end());
	return std::string(b, e);
    }
    std::string to_string(std::string_view input) const {
	return std::string("\"") + std::string(input) + "\"";
    }
};

template<>
struct lexical_cast_impl<const char*> {
    const char* convert(std::string_view input) const {
	return input.begin();
    }
	
    std::string to_string(const char *input) const {
	return {input};
    }
};

template<size_t N>
struct lexical_cast_impl<char[N]> {
    const char* convert(std::string_view input) const {
	return input.begin();
    }
    std::string to_string(const char *input) const {
	return {input};
    }
};

}; // core::lexical_cast_detail

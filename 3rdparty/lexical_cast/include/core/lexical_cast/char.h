// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include "impl.h"
#include "error.h"

namespace core::lexical_cast_detail {

template<>
struct lexical_cast_impl<char> {
    char convert(std::string_view input) const {
	if (input.size() != 1)
	    throw lexical_cast_error(input, "char");
	return input[0];
    }
	
    std::string to_string(char input) const {
	return {input};
    }
};

}; // core::lexical_cast_detail

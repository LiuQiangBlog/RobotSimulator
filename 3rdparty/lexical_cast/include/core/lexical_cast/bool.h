// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include "impl.h"
#include "error.h"
#include "util.h"

namespace core::lexical_cast_detail {

template<>
struct lexical_cast_impl<bool> {
    bool convert(std::string_view raw_input) const {
	std::string_view input = unwrap_ws(raw_input);
	if (input == "0" or input == "f" or input == "F" or input == "false")
	    return false;
	if (input == "1" or input == "t" or input == "T" or input == "true")
	    return true;
	throw lexical_cast_error(input, "bool");
    }
	
    std::string to_string(bool input) const {
	if (input) return "t";
	else return "f";
    }
};

}; // core::lexical_cast_detail

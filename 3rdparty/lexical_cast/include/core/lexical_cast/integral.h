// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023, 2024 by Mark Melton
//

#pragma once
#include <charconv>
#include "impl.h"
#include "error.h"
#include "util.h"

namespace core::lexical_cast_detail {

template<std::integral T>
T parse_integral(std::string_view input, std::string_view name) {
    try {
	T value{0};
	int base{10};

	// Ignore leading plus sign because *std::from_chars* does not
	// allow it.
	if (input.size() > 0 and input.front() == '+')
	    input = input.substr(1);

	// Recognize hex format prefix, i.e. 0x.
	if ((input.size() > 1) and (input[0] == '0') and
	    ((input[1] == 'x') or input[1] == 'X')) {
	    input = input.substr(2);
	    base = 16;
	}
	
	auto r = std::from_chars(input.begin(), input.end(), value, base);
	if (r.ptr != input.end())
	    throw lexical_cast_error(input, name);
	return value;
    }
    catch (std::invalid_argument const&) {
	throw lexical_cast_error(input, name);
    }
    catch (std::out_of_range const&) {
	throw lexical_cast_error(input, name);
    }
}

template<std::integral T>
struct lexical_cast_impl<T> {
    T convert(std::string_view input) const {
	return parse_integral<T>(unwrap_ws(input), "integral");
    }
    
    std::string to_string(T input) const {
	return std::to_string(input);
    }
};

}; // core::lexical_cast_detail

// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <vector>
#include "builtin.h"
#include "util.h"

namespace core::lexical_cast_detail {

template<class T>
struct lexical_cast_impl<std::vector<T>> {
    std::vector<T> convert(std::string_view s) const {
	auto [iter, end] = unwrap(s.begin(), s.end());
	std::vector<T> vec;
	while (iter < end) {
	    auto delim = find_first(iter, end, ',');
	    vec.emplace_back(lexical_cast<T>({iter, delim}));
	    iter = delim + 1;
	}
	return vec;
    }

    std::string to_string(const std::vector<T>& vec) const {
	std::string r = "[";
	for (auto elem : vec) {
	    if (r.size() > 1)
		r += ",";
	    r += lexical_to_string(elem);
	}
	r += "]";
	return r;
    }
};

}; // core::lexical_cast_detail


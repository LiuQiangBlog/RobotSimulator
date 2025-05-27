// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <set>
#include "builtin.h"
#include "util.h"

namespace core::lexical_cast_detail {

template<class T>
struct lexical_cast_impl<std::set<T>> {
    std::set<T> convert(std::string_view input) const {
	auto [iter, end] = unwrap(input.begin(), input.end());
	std::set<T> s;
	while (iter < end) {
	    auto delim = find_first(iter, end, ',');
	    s.emplace(lexical_cast<T>({iter, delim}));
	    iter = delim + 1;
	}
	return s;
    }

    std::string to_string(const std::set<T>& s) const {
	std::string r = "[";
	bool sep{false};
	for (auto elem : s) {
	    if (sep)
		r += ",";
	    r += lexical_to_string(elem);
	    sep = true;
	}
	r += "]";
	return r;
    }
};

}; // core::lexical_cast_detail


// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <array>
#include "builtin.h"
#include "error.h"
#include "util.h"

namespace core::lexical_cast_detail {

template<class T, size_t N>
struct lexical_cast_impl<std::array<T, N>> {
    std::array<T, N> convert(std::string_view s) const {
	auto [iter, end] = unwrap(s.begin(), s.end());
	size_t idx{};
	std::array<T, N> arr{};
	while (iter < end) {
	    if (idx >= N)
		throw lexical_cast_error(s, "std::array", "too many elements");
	    auto delim = find_first(iter, end, ',');
	    arr[idx] = lexical_cast<T>({iter, delim});
	    iter = delim + 1;
	    ++idx;
	}
	if (idx < N)
	    throw lexical_cast_error(s, "std::array", "too few elements");
	return arr;
    }

    std::string to_string(const std::array<T, N>& arr) const {
	std::string r = "[";
	for (auto elem : arr) {
	    if (r.size() > 1)
		r += ",";
	    r += lexical_to_string(elem);
	}
	r += "]";
	return r;
    }
};

}; // core::lexical_cast_detail


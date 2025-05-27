// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <string>
#include <tuple>
#include <vector>
#include "builtin.h"
#include "error.h"
#include "util.h"

namespace core::lexical_cast_detail {

template<class T, size_t... I>
auto create_tuple(const std::vector<std::string_view>& fields, std::index_sequence<I...>) {
    return std::make_tuple(lexical_cast<std::tuple_element_t<I, T>>(fields[I])...);
}

template<size_t I, size_t N, class T>
void stringify_tuple(std::string& r, const T& tup) {
    r += lexical_to_string(std::get<I>(tup));
    if constexpr (I + 1 < N) {
	r += ",";
	stringify_tuple<I + 1, N>(r, tup);
    }
}

template<class... Ts>
struct lexical_cast_impl<std::tuple<Ts...>> {
    using Tuple = std::tuple<Ts...>;
    
    std::tuple<Ts...> convert(std::string_view s) const {
	constexpr auto N = sizeof...(Ts);
	
	auto [iter, end] = unwrap(s.begin(), s.end());
	std::vector<std::string_view> fields;
	size_t idx{};
	while (iter < end) {
	    if (idx >= N)
		throw lexical_cast_error(s, "too many elements for std::tuple");
	    auto delim = find_first(iter, end, ',');
	    fields.emplace_back(iter, delim);
	    iter = delim + 1;
	    ++idx;
	}
	if (idx < N)
	    throw lexical_cast_error(s, "too few elements for std::tuple");
	return create_tuple<Tuple>(fields, std::make_index_sequence<N>{});
    }

    std::string to_string(const Tuple& tup) const {
	std::string r = "(";
	stringify_tuple<0, sizeof...(Ts)>(r, tup);
	r += ")";
	return r;
    }
};

}; // core::lexical_cast_detail


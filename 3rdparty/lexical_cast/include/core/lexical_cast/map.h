// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <map>
#include "builtin.h"
#include "pair.h"
#include "util.h"

namespace core::lexical_cast_detail {

template<class K, class V>
struct lexical_cast_impl<std::map<K, V>> {
    std::map<K, V> convert(std::string_view s) const {
	auto [iter, end] = unwrap(s.begin(), s.end());
	std::map<K, V> m;
	while (iter < end) {
	    auto delim = find_first(iter, end, ',');
	    m.emplace(lexical_cast<std::pair<K,V>>({iter, delim}));
	    iter = delim + 1;
	}
	return m;
    }

    std::string to_string(const std::map<K, V>& m) const {
	std::string r = "{";
	for (const auto& pair : m) {
	    if (r.size() > 1)
		r += ",";
	    r += lexical_to_string(pair);
	}
	r += "}";
	return r;
    }
};

}; // core::lexical_cast_detail


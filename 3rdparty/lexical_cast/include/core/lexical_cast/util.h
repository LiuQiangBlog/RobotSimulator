// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023, 2024 by Mark Melton
//

#pragma once
#include <utility>
#include <cctype>
#include <string_view>

namespace core::lexical_cast_detail
{

inline constexpr char OpenCurly = '{';
inline constexpr char CloseCurly = '}';

inline constexpr char OpenBracket = '[';
inline constexpr char CloseBracket = ']';

inline constexpr char OpenParen = '(';
inline constexpr char CloseParen = ')';

inline constexpr char OpenQuote = '"';
inline constexpr char CloseQuote = '"';

template<class Iter>
Iter find_close(Iter iter, Iter end, char c) {
    while (iter < end) {
	switch (*iter) {
	case OpenCurly:
	    iter = find_close(++iter, end, CloseCurly);
	    break;

	case OpenBracket:
	    iter = find_close(++iter, end, CloseBracket);
	    break;

	case OpenParen:
	    iter = find_close(++iter, end, CloseParen);
	    break;

	default:
	    if (*iter == c)
		return iter;
	}
	++iter;
    }
    return iter;
}

template<class Iter>
Iter find_first(Iter iter, Iter end, char c) {
    while (iter < end) {
	switch (*iter) {
	case OpenCurly:
	    iter = find_close(++iter, end, CloseCurly);
	    break;

	case OpenBracket:
	    iter = find_close(++iter, end, CloseBracket);
	    break;

	case OpenParen:
	    iter = find_close(++iter, end, CloseParen);
	    break;

	default:
	    if (*iter == c)
		return iter;
	}
	++iter;
    }
    return iter;
}

template<class Iter>
bool matching_delims(Iter iter, Iter end) {
    if (*iter == OpenQuote and *(end - 1) == CloseQuote)
	return true;
    if (*iter == OpenCurly and end - 1 == find_close(iter + 1, end, CloseCurly))
	return true;
    if (*iter == OpenBracket and end - 1 == find_close(iter + 1, end, CloseBracket))
	return true;
    if (*iter == OpenParen and end - 1 == find_close(iter + 1, end, CloseParen))
	return true;
    return false;
}

template<class Iter>
std::pair<Iter,Iter> unwrap_ws(Iter begin, Iter end) {
    while (begin < end and std::isspace(*begin))
	++begin;
    while (end > begin and std::isspace(*(end - 1)))
	--end;
    return {begin, end};
}

inline std::string_view unwrap_ws(std::string_view str) {
    auto [b, e] = unwrap_ws(str.begin(), str.end());
    return {b, e};
}

template<class Iter>
std::pair<Iter,Iter> unwrap(Iter begin, Iter end) {
    auto [b, e] = unwrap_ws(begin, end);
    if (b < e and matching_delims(b, e)) {
	++b;
	--e;
    }
    return {b, e};
}

}; // core::lexical_cast_detail


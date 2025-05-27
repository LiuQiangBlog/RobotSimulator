// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <string>
#include <string_view>

namespace core {

namespace lexical_cast_detail {

template<class T>
struct lexical_cast_impl;

}; // lexical_cast_detail

/// Return an instance of `T` corresponding to the string `input`. Will
/// throw `lexical_cast_error` if `input` cannot be interpreted as an
/// instance of T.
template<class T>
T lexical_cast(std::string_view input) {
    return lexical_cast_detail::lexical_cast_impl<T>{}.convert(input);
}

/// Return a `std::string` that represents `value`. The returned
/// string can be passed to `lexical_cast<T>` to recover `value`.
template<class T>
std::string lexical_to_string(const T& value) {
    return lexical_cast_detail::lexical_cast_impl<T>{}.to_string(value);
}

}; // core

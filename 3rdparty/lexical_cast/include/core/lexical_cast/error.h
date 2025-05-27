// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023 by Mark Melton
//

#pragma once
#include <exception>
#include <string_view>
#include "fmt/format.h"

namespace core {

struct lexical_cast_error : public std::runtime_error {
    lexical_cast_error(std::string_view input, std::string_view type)
	: std::runtime_error(fmt::format("cannot parse '{:s}' as '{:s}'", input, type))
    { }

    lexical_cast_error(std::string_view input, std::string_view type,
		       std::string_view msg)
	: std::runtime_error(fmt::format("cannot parse '{:s}' as '{:s}': {}", input, type, msg))
    { }
};

}; // core

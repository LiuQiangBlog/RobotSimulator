// Copyright (C) 2017, 2018, 2019, 2021, 2022, 2023, 2024 by Mark Melton
//

#pragma once
#include <optional>

#include "builtin.h"
#include "error.h"
#include "util.h"

namespace core::lexical_cast_detail {

template <class T>
struct lexical_cast_impl<std::optional<T>> {
    std::optional<T> convert(std::string_view s) const {
        if (s.empty())
            return {};
        return lexical_cast<T>(s);
    }

    std::string to_string(const std::optional<T> &value) const {
        if (not value)
            return "";
        return lexical_to_string(*value);
    }
};

}; // namespace core::lexical_cast_detail

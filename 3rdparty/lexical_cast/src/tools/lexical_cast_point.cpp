// Copyright (C) 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include "core/lexical_cast/vector.h"
#include "core/lexical_cast/pair.h"

using std::cout, std::endl;

struct Point {
    double x, y;
    bool operator==(const Point&) const = default;
};

namespace core::lexical_cast_detail {

template<>
struct lexical_cast_impl<Point> {
    Point convert(std::string_view input) const {
	auto [x, y] = lexical_cast<std::pair<double,double>>(input);
	return Point{x, y};
    }

    std::string to_string(const Point& p) const {
	return lexical_to_string(std::make_pair(p.x, p.y));
    }
};

}; // lexical_cast_detail

int main(int argc, const char *argv[]) {
    using Vector = std::vector<Point>;
    
    auto text = "[(1.0,2.0), (3.0,4.0)]";
    auto v0 = core::lexical_cast<Vector>(text);
    assert(core::lexical_cast<Vector>(core::lexical_to_string(v0)) == v0);

    Vector v1 = { Point{1.0, 2.0}, Point{3.0, 4.0} };
    assert(v0 == v1);
    
    return 0;
}

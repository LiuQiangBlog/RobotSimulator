// Copyright (C) 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include "core/lexical_cast/stdlib.h"

using std::cout, std::endl;

struct Person {
    std::string name;
    int age;
    double height;
    bool operator==(const Person&) const = default;
};

namespace core::lexical_cast_detail {

template<>
struct lexical_cast_impl<Person> {
    Person convert(std::string_view input) const {
	auto [name, age, height] = lexical_cast<std::tuple<std::string,int,double>>(input);
	return Person{name, age, height};
    }

    std::string to_string(const Person& p) const {
	return lexical_to_string(std::make_tuple(p.name, p.age, p.height));
    }
};

}; // lexical_cast_detail

int main(int argc, const char *argv[]) {
    using Vector = std::vector<Person>;
    
    auto text = "[(bob,27,1.65), (alice,25,1.45)]";
    auto v0 = core::lexical_cast<Vector>(text);
    assert(core::lexical_cast<Vector>(core::lexical_to_string(v0)) == v0);

    Vector v1 = { Person{"bob", 27, 1.65}, Person{"alice", 25, 1.45} };
    assert(v0 == v1);
    
    return 0;
}

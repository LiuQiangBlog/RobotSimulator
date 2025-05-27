// Copyright (C) 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include "core/lexical_cast/string.h"
#include "core/lexical_cast/map.h"

using std::cout, std::endl;

int main(int argc, const char *argv[]) {
    using Map = std::map<std::string,int>;
    
    auto text = R"({["a", 1], ["b", 2]})";
    auto m0 = core::lexical_cast<Map>(text);
    assert(core::lexical_cast<Map>(core::lexical_to_string(m0)) == m0);

    Map m1 = {{"a", 1}, {"b", 2}};
    assert(m0 == m1);
    
    return 0;
}

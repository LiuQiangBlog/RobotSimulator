// Copyright (C) 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include "core/lexical_cast/builtin.h"

using namespace core;
using std::cout, std::endl;

int main(int argc, const char *argv[]) {

    auto b0 = core::lexical_cast<bool>("f");
    auto b1 = core::lexical_cast<bool>("F");
    auto b2 = core::lexical_cast<bool>("false");
    assert(b0 == b1 == b2 == false);
    
    auto b3 = core::lexical_cast<bool>("t");
    auto b4 = core::lexical_cast<bool>("T");
    auto b5 = core::lexical_cast<bool>("true");
    assert(b3 == b4 == b5 == true);

    auto c = core::lexical_cast<char>("a");
    assert(c == 'a');

    auto f = core::lexical_cast<double>("12.3");
    assert(f == 12.3);

    auto i = core::lexical_cast<int>("123");
    assert(i == 123);
    
    return 0;
}

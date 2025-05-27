// Copyright (C) 2023 by Mark Melton
//

#undef NDEBUG
#include <cassert>

#include <iostream>
#include "core/lexical_cast/array.h"
#include "core/lexical_cast/map.h"
#include "core/lexical_cast/pair.h"
#include "core/lexical_cast/set.h"
#include "core/lexical_cast/string.h"
#include "core/lexical_cast/tuple.h"
#include "core/lexical_cast/vector.h"

using namespace core;
using std::cout, std::endl;

int main(int argc, const char *argv[]) {

    auto a = core::lexical_cast<std::array<int,3>>("[1,2,3]");
    assert(a[0] == 1 and a[1] == 2 and a[2] == 3);

    auto m = core::lexical_cast<std::map<char,int>>("{(a,1),(b,2)}");
    assert(m['a'] == 1 and m['b'] == 2);

    auto p = core::lexical_cast<std::pair<char,int>>("(a,1)");
    assert(p.first == 'a' and p.second == 1);

    auto s = core::lexical_cast<std::set<int>>("[1,2,3]");
    assert(s.size() == 3 and s.contains(1) and s.contains(2) and s.contains(3));

    auto str = core::lexical_cast<std::string>(R"("abc")");
    assert(str == "abc");

    auto t = core::lexical_cast<std::tuple<char,int,double>>("(a,1,2.0)");
    assert(t == std::make_tuple('a', 1, 2.0));

    auto v0 = core::lexical_cast<std::vector<int>>("[1, 2, 3]");
    assert(v0.size() == 3 and v0[0] == 1 and v0[1] == 2 and v0[2] == 3);
    
    auto v1 = core::lexical_cast<std::vector<int>>("1, 2, 3 ");
    assert(v1.size() == 3 and v1[0] == 1 and v1[1] == 2 and v1[2] == 3);
    return 0;
}

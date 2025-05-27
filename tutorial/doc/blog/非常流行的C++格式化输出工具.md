https://github.com/fmtlib/fmt fmt库天然的支持C++标准库容器的输出操作，并且支持用户指定间隔符号

https://hackingcpp.com/cpp/libs/fmt.html

```C++
#include <vector>
#include <array>
#include <fmt/ranges.h>
std::vector<double> v {1.2, 5.6, 7.8};
std::array<int,4> a {2, 3, 4, 5};
fmt::print("v: {}\n", v);
fmt::print("a: {}\n", a);
fmt::print("{}\n", fmt::join(v,"|"));
```

```angular2html
v: [1.2, 5.6, 7.8]
a: [2, 3, 4, 5]
1.2|5.6|7.8
```



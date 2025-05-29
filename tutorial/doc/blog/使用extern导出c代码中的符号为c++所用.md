extern

编译动态库时的特殊注意事项

当编译 C 代码为动态库并在 C++ 中使用时，还需要注意以下几点：

在 C 代码中使用 __declspec(dllexport)（Windows 平台）

在 Windows 上，导出动态库中的函数需要使用 __declspec(dllexport)：

```c
// zcm.h (Windows 版本)
#ifdef ZCM_EXPORTS
#define ZCM_API __declspec(dllexport)
#else
#define ZCM_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

ZCM_API void zcm_create(...);
ZCM_API void zcm_publish(...);

#ifdef __cplusplus
}
#endif
```

在编译动态库时，定义 ZCM_EXPORTS 宏；在使用该库的 C++ 代码中，不定义这个宏。

在 Linux 平台上使用 -fPIC 选项

编译动态库时，需要使用 -fPIC（Position Independent Code）选项：

```shell
gcc -fPIC -c zcm.c -o zcm.o
gcc -shared -o libzcm.so zcm.o
```

如果使用 CMake 构建，可以这样设置：
```cmake
find_library(ZCM_LIBRARY zcm)
target_link_libraries(your_cpp_project ${ZCM_LIBRARY})
```

对于运行时找不到动态库的问题，可以：
将动态库路径添加到 LD_LIBRARY_PATH 环境变量（Linux）
```c
// example_c_library.h
#ifndef EXAMPLE_C_LIBRARY_H
#define EXAMPLE_C_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

int add(int a, int b);

#ifdef __cplusplus
}
#endif

#endif // EXAMPLE_C_LIBRARY_H

// example_c_library.c
int add(int a, int b) {
    return a + b;
}
```

```shell
gcc -fPIC -c example_c_library.c -o example_c_library.o
gcc -shared -o libexample_c_library.so example_c_library.o
```
在C++中使用：
```c++
// main.cpp
#include <iostream>
extern "C" {
    #include "example_c_library.h"
}

int main() {
    std::cout << "Result: " << add(3, 4) << std::endl;
    return 0;
}
```

编译 C++ 程序并链接动态库：
```shell
g++ main.cpp -o test -L. -lexample_c_library
```

运行时确保动态库可被找到：
```shell
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:.
./test
```
通过以上方法，你应该能够解决在 C++ 中使用 C 语言编写的动态库时遇到的符号导出和链接问题。

__declspec(dllexport) 是 Windows 平台特有的语法，用于指定动态链接库（DLL）中需要导出的函数和变量。
在 Linux 或 macOS 等类 Unix 系统中，动态库（.so 文件）的导出机制完全不同，不需要这个语法。

跨平台兼容的做法
如果你需要编写同时支持 Windows 和 Linux 的代码，可以使用预处理器指令来区分平台：
```shell
// your_library.h
#ifndef YOUR_LIBRARY_H
#define YOUR_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #ifdef YOUR_LIBRARY_EXPORTS
        #define YOUR_LIBRARY_API __declspec(dllexport)
    #else
        #define YOUR_LIBRARY_API __declspec(dllimport)
    #endif
#else
    #define YOUR_LIBRARY_API
#endif

// 使用 YOUR_LIBRARY_API 宏声明需要导出的函数
YOUR_LIBRARY_API void your_function();

#ifdef __cplusplus
}
#endif

#endif
```

在 CMake 中，可以这样设置：
```cmake
# 设置生成位置无关代码
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 定义导出宏（仅适用于 Windows）
if(WIN32)
    add_definitions(-DYOUR_LIBRARY_EXPORTS)
endif()

# 添加动态库
add_library(your_library SHARED your_c_code.c)
```

总结
Windows：需要 __declspec(dllexport) 来显式导出符号，同时需要在 C++ 中使用 extern "C"。
Linux/macOS：不需要 __declspec(dllexport)，只需要 -fPIC 选项和 extern "C"。
跨平台代码：使用预处理器指令结合平台特定的导出机制。
你在 Ubuntu 上的做法是正确的，只需要确保使用 -fPIC 编译选项和正确的 extern "C" 声明即可。
//
// Created by liuqiang on 25-5-10.
//
#include <cpuid.h>
#include <iostream>

int main()
{
    unsigned int eax, ebx, ecx, edx;
    // 使用leaf 1而非leaf 0x80000001
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    if (ecx & (1 << 13))
    { // 检查ECX第13位
        std::cout << "CMPXCHG16B supported" << std::endl;
    }
    else
    {
        std::cout << "CMPXCHG16B NOT supported" << std::endl;
    }
    return 0;
}
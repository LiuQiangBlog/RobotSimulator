//
// Created by liuqiang on 25-5-24.
//
#include <vector>
#include <Logging.h>

int main()
{
    std::vector<double> vec{1,2,3,4};
    CLOG_INFO << vec.front();
    CLOG_INFO << vec.back();
    return 0;
}
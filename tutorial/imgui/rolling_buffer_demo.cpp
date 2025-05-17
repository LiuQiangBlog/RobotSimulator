//
// Created by liuqiang on 25-5-17.
//

#include "rolling_buffer.h"
#include "Logging.h"

int main()
{
    RollingBuffer<double> buffer(10000);

    // 添加15000个数据点
    for (int i = 0; i < 15000; ++i)
    {
        buffer.push_back(i * 0.1);
    }

    // 获取连续内存数据进行处理
    const auto &data = buffer.data();
    CLOG_INFO << data.size();
    return 0;
}
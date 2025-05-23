//
// Created by liuqiang on 25-5-13.
//
#include <zcm/zcm-cpp.hpp>
#include "types/example_t.hpp"
#include <iostream>

class Handler
{
public:
    void handle(const zcm::ReceiveBuffer *rbuf, const std::string &chan, const example_t *msg)
    {
        std::cout << "[回放] 接收到消息: " << msg->name << " 时间戳: " << msg->timestamp << " 位置: ("
                  << msg->position[0] << ", " << msg->position[1] << ", " << msg->position[2] << ")" << std::endl;
    }
};

int main()
{
    zcm::RegisterAllPlugins();
    zcm::ZCM zcm("udpm://239.255.76.67:7667?ttl=1,file:///home/liuqiang/log.zcm?mode=w");

    if (!zcm.good())
    {
        std::cerr << "ZCM 初始化失败（file://）" << std::endl;
        return 1;
    }

    Handler h;
    zcm.subscribe("EXAMPLE", &Handler::handle, &h);

    std::cout << "开始回放..." << std::endl;
    zcm.run(); // 自动播放文件中的所有消息

    return 0;
}
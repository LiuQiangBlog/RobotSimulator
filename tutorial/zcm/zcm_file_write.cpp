//
// Created by liuqiang on 25-5-13.
//
#include <zcm/zcm-cpp.hpp>
#include "types/example_t.hpp"
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <thread>
using std::string;

inline int64_t zcm_now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

class Handler
{
public:
    ~Handler() {}

    void handleMessage(const zcm::ReceiveBuffer *rbuf, const string &chan, const example_t *msg)
    {
        printf("Received message on channel \"%s\":\n", chan.c_str());
        printf("  timestamp   = %lld\n", (long long)msg->timestamp);
        printf("  position    = (%f, %f, %f)\n", msg->position[0], msg->position[1], msg->position[2]);
        printf("  orientation = (%f, %f, %f, %f)\n", msg->orientation[0], msg->orientation[1], msg->orientation[2],
               msg->orientation[3]);
        printf("  ranges:");
        for (int i = 0; i < msg->num_ranges; i++)
            printf(" %d", msg->ranges[i]);
        printf("\n");
        printf("  name        = '%s'\n", msg->name.c_str());
        printf("  enabled     = %d\n", msg->enabled);
    }
};

int main()
{
    registerAllPlugins();

    // ✅ 设置多 transport：既发 UDP，又记录日志
    zcm::ZCM zcm("udpm://239.255.76.67:7667?ttl=1,file:///home/liuqiang/log.zcm?mode=w");

    if (!zcm.good())
    {
        std::cerr << "ZCM 初始化失败" << std::endl;
        return 1;
    }
    Handler handlerObject;
    zcm.subscribe("EXAMPLE", &Handler::handleMessage, &handlerObject);
    std::thread zcmThread([&]() {
                              zcm.run(); // 必须让它持续运行以触发接收和日志记录
                          });
    example_t my_data{};
    my_data.timestamp = 0;

    my_data.position[0] = 1;
    my_data.position[1] = 2;
    my_data.position[2] = 3;

    my_data.orientation[0] = 1;
    my_data.orientation[1] = 0;
    my_data.orientation[2] = 0;
    my_data.orientation[3] = 0;

    // my_data.num_ranges = 100000;
    my_data.num_ranges = 15;
    my_data.ranges.resize(my_data.num_ranges);
    for (int i = 0; i < my_data.num_ranges; i++)
        my_data.ranges[i] = i % 20000;

#if !defined(__clang__)
    my_data.name = my_data.test_const_string;
#endif

    my_data.enabled = true;

    for (int i = 0; i < 5; ++i)
    {
        my_data.timestamp = zcm_now();
        zcm.publish("EXAMPLE", &my_data);
        zcm.handleNonblock();
        std::cout << "[写入] 发布消息: " << my_data.name << " 时间戳: " << my_data.timestamp << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    zcm.stop();
    zcmThread.join();
    return 0;
}
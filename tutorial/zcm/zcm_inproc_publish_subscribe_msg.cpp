//
// Created by liuqiang on 25-5-12.
//
#include <unistd.h>
#include <zcm/zcm-cpp.hpp>
#include <zcm/transport_register.hpp>
#include <zcm/transport/udp/udp.hpp>
#include "types/example_t.hpp"

#include <poll.h>

class Handler
{
public:
    void handleMessage(const zcm::ReceiveBuffer *rbuf, const std::string &chan, const example_t *msg)
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

inline int64_t zcm_now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

int main(int argc, char *argv[])
{
    registerAllPlugins();
    zcm::ZCM zcm("inproc");

    if (!zcm.good())
    {
        return 1;
    }

    Handler handlerObject;
    zcm.subscribe("EXAMPLE", &Handler::handleMessage, &handlerObject); // ✅ 只订阅一次！

    example_t my_data{};
    my_data.timestamp = zcm_now();
    my_data.position[0] = 1;
    my_data.position[1] = 2;
    my_data.position[2] = 3;

    my_data.orientation[0] = 1;
    my_data.orientation[1] = 0;
    my_data.orientation[2] = 0;
    my_data.orientation[3] = 0;

    my_data.num_ranges = 5;
    my_data.ranges.resize(my_data.num_ranges);
    for (int i = 0; i < my_data.num_ranges; i++)
        my_data.ranges[i] = i;

    my_data.name = "test";
    my_data.enabled = true;
    //    std::thread zcm_thread(
    //        [&]()
    //        {
    //            zcm.run(); // 持续阻塞处理消息
    //        });
    for (int i = 0; i < 10; i++)
    {
        zcm.publish("EXAMPLE", &my_data); // ✅ 发送
        zcm.handleNonblock();             // ✅ 调用 handle 处理消息（非阻塞）
        for (auto &val : my_data.position)
            val++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
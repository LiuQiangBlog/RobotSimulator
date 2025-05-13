//
// Created by liuqiang on 25-5-12.
//
#include <unistd.h>
#include <zcm/zcm-cpp.hpp>
#include <zcm/transport_register.hpp>
#include <zcm/transport/udp/udp.hpp>
#include "types/example_t.hpp"
#include "types/all_channels_t.hpp"
#include <set>

inline int64_t zcm_now()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

int main(int argc, char *argv[])
{
    zcm::RegisterAllPlugins();
    zcm::ZCM zcm("ipc");
//    zcm::ZCM zcm("udp://127.0.0.1:9000:9001?ttl=1");
//    zcm::ZCM zcm("ipcshm://");
//    zcm::ZCM zcm("udpm://239.255.76.67:7654?ttl=1");
    if (!zcm.good())
        return 1;

    std::string channel = "EXAMPLE";
    if (argc > 1)
        channel = argv[1];

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
    all_channels_t data;
    data.channels.push_back(channel);
    data.channels.emplace_back("aaa");
    data.channels.emplace_back("bbb");
    data.cnt = 3;
    data.timestamp = zcm_now();

    bool published{false};
    while (1)
    {
        zcm.publish(channel, &my_data);
        zcm.publish("aaa", &my_data);
        zcm.publish("bbb", &my_data);
        for (auto &val : my_data.position)
            val++;

        if (!published)
        {
            zcm.publish("channels", &data);
            published = true;
        }
        usleep(1000 * 1000);
    }

    return 0;
}

//
// Created by liuqiang on 25-5-12.
//
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <zcm/zcm-cpp.hpp>
#include "types/example_t.hpp"
#include "types/all_channels_t.hpp"
#include <set>
#include <Logging.h>

using std::string;

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
    void handleMessage2(const zcm::ReceiveBuffer *rbuf, const string &chan, const all_channels_t *msg)
    {
        if (already_received)
        {
            return;
        }
        for (auto &channel : msg->channels)
        {
            discovered_channels.insert(channel);
        }
        if (zcmPtr && channelsSubed)
        {
            zcmPtr->unsubscribe(channelsSubed);
            channelsSubed = nullptr;
            already_received = true; // 设置标志位
            CLOG_INFO << "unsubscribe channels success.";
        }
    }
    std::set<std::string> discovered_channels;
    bool already_received = false;
    zcm::Subscription *channelsSubed = nullptr;
    zcm::ZCM *zcmPtr;
};

int main(int argc, char *argv[])
{
    zcm::RegisterAllPlugins();
    zcm::ZCM zcm("ipc");
    //    zcm::ZCM zcm("udp://127.0.0.1:9001:9000?ttl=1");
    //    zcm::ZCM zcm("ipcshm://");
    //    zcm::ZCM zcm("udpm://239.255.76.67:7654?ttl=1");
    if (!zcm.good())
        return 1;

    Handler hd;
    hd.zcmPtr = &zcm;
    zcm.subscribe("EXAMPLE", &Handler::handleMessage, &hd);
    hd.channelsSubed = zcm.subscribe("channels", &Handler::handleMessage2, &hd);

    zcm.run();
    CLOG_INFO << "channels: ";
    for (const auto &ch : hd.discovered_channels)
    {
        std::cout << "- " << ch << std::endl;
    }
    return 0;
}

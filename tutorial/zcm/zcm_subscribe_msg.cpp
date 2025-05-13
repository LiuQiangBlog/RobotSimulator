//
// Created by liuqiang on 25-5-12.
//
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <zcm/zcm-cpp.hpp>
#include "types/example_t.hpp"
using std::string;
#include <set>
#include <Logging.h>

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
    void handleMessage2(const zcm::ReceiveBuffer *rbuf, const string &chan, const example_t *msg)
    {
        CLOG_INFO << "channel: " << chan;
        channels.insert(chan);
    }
    std::set<std::string> channels;
};

int main(int argc, char *argv[])
{
    registerAllPlugins();
    zcm::ZCM zcm("ipc");
    //    zcm::ZCM zcm("udp://127.0.0.1:9001:9000?ttl=1");
    //    zcm::ZCM zcm("ipcshm://");
    //    zcm::ZCM zcm("udpm://239.255.76.67:7654?ttl=1");
    if (!zcm.good())
        return 1;

    Handler handlerObject;
    zcm.subscribe("EXAMPLE", &Handler::handleMessage, &handlerObject);
    zcm.subscribe("*", &Handler::handleMessage2, &handlerObject);
    zcm.run();

    return 0;
}

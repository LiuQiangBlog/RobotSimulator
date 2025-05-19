//
// Created by liuqiang on 25-5-12.
//
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <zcm/zcm-cpp.hpp>
#include "types/example_t.hpp"
#include "types/all_channels_t.hpp"
#include "types/all_timed_value.hpp"
#include <set>
#include <Logging.h>

using std::string;

class Handler
{
public:
    ~Handler() {}

    void handle(const zcm::ReceiveBuffer *buf, const string &channel, const all_timed_value *msg)
    {
        CLOG_INFO << "channel: " << channel;
        CLOG_INFO << "msg->cnt: " << msg->cnt;
    }
};

int main(int argc, char *argv[])
{
    zcm::RegisterAllPlugins();
    zcm::ZCM zcm("ipcshm://");
    if (!zcm.good())
    {
        return -1;
    }
    Handler hd;
    zcm.subscribe("channel", &Handler::handle, &hd);
    zcm.run();
    zcm.stop();
    return 0;
}

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
#include <thread>

using std::string;

int main(int argc, char *argv[])
{
    zcm::RegisterAllPlugins();
    zcm::ZCM zcm("ipcshm://");
    if (!zcm.good())
    {
        return -1;
    }
    int cnt = 100;
    timed_value data;
    all_timed_value all_value;
    while (true)
    {
        data.name = "pos/x";
        data.timestamp = 0;
        data.value = 1;
        all_value.channels.push_back(data);
        all_value.cnt = int(all_value.channels.size());
        zcm.publish("channel", &all_value);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    zcm.run();
    zcm.stop();
    return 0;
}

//
// Created by liuqiang on 25-5-12.
//

#include <data_tamer/sinks/publish_sink.hpp>

int main()
{
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto nanoseconds = std::chrono::nanoseconds(duration);
    auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(nanoseconds).count();
    CLOG_INFO << seconds;
    return 0;
}
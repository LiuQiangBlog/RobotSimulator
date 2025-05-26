//
// Created by liuqiang on 25-5-26.
//
#include <UUID.h>
#include <Logging.h>

int main()
{
    UUID uuid_v1{UUID::Version::v1};
    CLOG_INFO << uuid_v1.str();
    CLOG_INFO << uuid_v1.str();
    return 0;
}